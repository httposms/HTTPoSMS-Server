#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pthread.h>

#include "common.h"
#include "log.h"
#include "zlib.h"

#define MAX_PARALLEL 10

static int _net_add_job(const char*);

static pthread_t net_th;
static CURLM *mhandle;

/* We recieve requests on in_fd, read them from req_fd and send completed requests on the response_fd */
static int req_fd, response_fd, in_fd;

struct net_response {
        size_t len;
        uint8_t *ptr;
};

static inline struct net_response *response_init() 
{
        return (struct net_response *)calloc(sizeof(struct net_response), 1);
}

static inline void response_free(struct net_response *arg)
{
        if(!arg)
                return;

        free(arg->ptr);
        free(arg);
}

size_t resp_handler(uint8_t * data, size_t size, size_t nmemb, void *userp)
{
        const size_t chunksz = size * nmemb;
        struct net_response *arg = (struct net_response*)userp;        

        /* 0 bytes processed, not success */
        if(!arg)
                return 0;

        uint8_t *ptr = realloc(arg->ptr, arg->len + chunksz + 1);
        arg->ptr = ptr;
        memcpy(arg->ptr + arg->len, data, chunksz);
        arg->len += chunksz;
        arg->ptr[arg->len] = '\0';

        return chunksz;        
}

int non_text_tag(const xmlChar *name) {
        if(!xmlStrcmp(name, (xmlChar *)"meta")) {
                return 1;
        }
        if(!xmlStrcmp(name, (xmlChar *)"link")) {
                return 1;
        }
        if(!xmlStrcmp(name, (xmlChar *)"script")) {
                return 1;
        }
        if(!xmlStrcmp(name, (xmlChar *)"audio")) {
                return 1;
        }
        if(!xmlStrcmp(name, (xmlChar *)"video")) {
                return 1;
        }
        if(!xmlStrcmp(name, (xmlChar *)"title")) {
                return 1;
        }
        return 0;
}

static int postprocess_elem(htmlNodePtr elem) {
        while (elem) {
                if (elem->type == XML_ELEMENT_NODE) {
                        if(non_text_tag(elem->name)) {
                                LOG_DBUG("removing node\n");
                                htmlNodePtr swp;
                                xmlUnlinkNode(elem);
                                swp = elem->next;
                                xmlFreeNode(elem);
                                elem = swp;
                                continue;
                        }
                }
                if (elem->type == HTML_COMMENT_NODE){
                        LOG_DBUG("Remove html comment\n");
                        htmlNodePtr swp;
                        xmlUnlinkNode(elem);
                        swp = elem->next;
                        xmlFreeNode(elem);
                        elem = swp;
                        continue;
                }

                postprocess_elem(elem->children);
                elem = elem->next;
        }
        return 0;
}

int postprocess_handler(struct net_response *resp)
{

        /* Remove non text nodes from the tree, compress and send off */
        htmlDocPtr doc = NULL;
        htmlNodePtr node = NULL;
        doc = htmlReadMemory(
                        (char *)resp->ptr,
                        resp->len,
                        "example.com",
                        "UTF-8",
                        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | XML_PARSE_COMPACT | XML_PARSE_NOBLANKS
                        );

        if(!doc)
                goto err;


        node = xmlDocGetRootElement(doc);
        postprocess_elem(node);

        xmlChar *trimmed = NULL;
        int sz = 0;
        htmlDocDumpMemoryFormat(doc, &trimmed, &sz, 0);
        void * compressed = calloc(sz, 1);


        z_stream defstream;
        defstream.zalloc = Z_NULL;
        defstream.zfree = Z_NULL;
        defstream.opaque = Z_NULL;
        
        defstream.avail_in = sz;
        defstream.next_in = (Bytef *)trimmed;
        defstream.avail_out = sz;
        defstream.next_out = compressed;

        deflateInit(&defstream, Z_BEST_COMPRESSION);
        deflate(&defstream, Z_FINISH);
        deflateEnd(&defstream);

        LOG_DBUG("sz before: %lu, sz after: %lu\n", resp->len, defstream.total_out);

        xmlFreeDoc(doc);
        response_free(resp);
        free(trimmed);
        free(compressed);
        return 0;

err:
        LOG_WARN("Failed to postprocess page");
        xmlFreeDoc(doc);
        return 1;
}

void *req_handler() {
        /* TODO: handle < sizeof(ptr) reads */
        void *ret;
        int sz = read(req_fd, &ret, sizeof(ret));
        LOG_DBUG("Got %p, sz: %d from ingress\n", ret, sz);
        return ret;
}

static void *net_loop(void *arg)
{
        LOG_DBUG("Started net loop\n");
        CURLMsg *msg;
        int msgs_left = -1;

        do {
                int still_running = 1; 


                while(curl_multi_perform(mhandle, &still_running) ==
                                CURLM_CALL_MULTI_PERFORM);


                while((msg = curl_multi_info_read(mhandle, &msgs_left))) {
                        if(msg->msg == CURLMSG_DONE) {
                                struct net_response *resp;
                                CURL *e = msg->easy_handle;
                                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &resp);
                                postprocess_handler(resp);
                                curl_multi_remove_handle(mhandle, e);
                                curl_easy_cleanup(e);
                        }
                }

                int max_fd = -1;
                struct timeval timeout;
                timeout.tv_sec = 1; /* 1 second */
                timeout.tv_usec = 0;
                fd_set readers, writers, errors;
                FD_ZERO(&readers);
                FD_ZERO(&writers);
                FD_ZERO(&errors);

                curl_multi_fdset(mhandle, &readers, &writers, &errors, &max_fd);
                FD_SET(req_fd, &readers);
                max_fd = (req_fd > max_fd) ? req_fd : max_fd;
                select(max_fd + 1, &readers, &writers, &errors, &timeout);

                if(FD_ISSET(req_fd, &readers)){
                        LOG_DBUG("Hit net request handler\n");
                        _net_add_job(req_handler());
                }

        } while(1);

        LOG_WARN("net component exiting\n");
        return NULL;
}

int net_init(const int *io_fd, const int writer_fd) 
{
        req_fd = io_fd[READ_FD];
        in_fd = io_fd[WRITE_FD];
        response_fd = writer_fd;

        CURLcode err = curl_global_init(CURL_GLOBAL_ALL);        
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s", curl_easy_strerror(err));
                return err;
        }

        mhandle = curl_multi_init();
        if(!mhandle)
                LOG_INFO("did not create mhandle\n");
        curl_multi_setopt(mhandle, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);
        pthread_create(&net_th, NULL, &net_loop, NULL);
        return 0;
}

/* 
 * queue a job for execution by the net component
 * 
 * This is a non-blocking / lock free interface
 *
 * Simply write a pointer to the request to a pipe that
 * the component waits on
 *
 * */
int net_add_job(char *url)
{
        int sz = write(in_fd, &url, sizeof(url));
        LOG_INFO("Recieved request at net component %p, sz: %d\n", url, sz);
        return 0;
}

/*
 * Actually queue the job to be worked on
 *
 * Read a pointer from the request fd, and submit
 * the URL to curl
 * */
int _net_add_job(const char *url)
{
        int err;
        struct net_response *resp = NULL;
        CURL *job = NULL;

        resp = response_init();
        if(!resp) {
                LOG_FAIL("Could not initialise response\n");
                goto err;
        }
        job = curl_easy_init();
        if(!job)
        {
                LOG_FAIL("Could not initialise libcurl, easy_init returned NULL\n");
                goto err;        
        }
        err = curl_easy_setopt(job, CURLOPT_WRITEFUNCTION, &resp_handler);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }
        err = curl_easy_setopt(job, CURLOPT_WRITEDATA, resp);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }

        err = curl_easy_setopt(job, CURLOPT_CLOSESOCKETDATA, resp);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }



        err = curl_easy_setopt(job, CURLOPT_URL, url);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }

        err = curl_easy_setopt(job, CURLOPT_PRIVATE, resp);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }

        err = curl_multi_add_handle(mhandle, job);
        if(err)
        {
                LOG_FAIL("Could not initialise libcurl: %s\n", curl_easy_strerror(err));
                goto err;
        }

        LOG_INFO("job added to net component handle\n");
        return 0;
err:
        response_free(resp);
        curl_easy_cleanup(job);
        return 1;
}

void net_exit()
{
        curl_multi_cleanup(mhandle);
        curl_global_cleanup();
}
