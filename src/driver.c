#define _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>

#include "common.h"
#include "log.h"

#define DEFAULT_DEVICE "/dev/ttyAMA0"
#define DEFAULT_LOG "/tmp/httposms.log"
#define STRINGIZE(x) #x
#define STR(x) STRINGIZE(x)


static void usage();
static void mkdaemon();

int main(int argc, char *argv[])
{
        int daemonize = 1;
        int logfd = STDOUT_FILENO;
        loglevel verbosity = DEBUG;
        char *device = DEFAULT_DEVICE;

        static struct option longopts[] = {
                {"version", no_argument, NULL, 'v'},
                {"log-level", required_argument, NULL, 'l'},
                {"no-daemon", no_argument, NULL, 'n'},
                {"device", required_argument, NULL, 'd'},
                {"help", no_argument, NULL, 'h'},
                {0, 0, 0, 0},
        };

        while(1)
        {
                int optin, curr;
                curr = getopt_long(argc, argv, "vnl:d:h", longopts, &optin);
                if(curr < 0)
                        break;
                switch(curr) {
                        case 'v':
                                printf(STR(VERSION) "\n");
                                break;
                        case 'n':
                                daemonize = 0;
                                break;
                        case 'd':
                                device = optarg;
                                break;
                        case 'l':
                                verbosity = ctoll(optarg);
                                break;
                        case 'h':
                                usage();
                                exit(NO_ERROR);
                                break;
                        case '?':
                                usage();
                                exit(MINOR_ERROR);
                                break;
                }
        }

        if(daemonize){
                mkdaemon();
                logfd = open(DEFAULT_LOG, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        }
        
        log_init(logfd, verbosity);

        return 0;
}

static void usage()
{
        printf(
                        "Usage: " STR(NAME) " [OPTIONS]... [DEVICE not implemented]\n"
                        "Using the GPRS/GSM modem represented by DEVICE\n"
                        "tunnel HTTP traffic over SMS from requests to the modem\n"
                        "By default the process will attempt to daemonize, and write its' "
                        "output to the file \"" DEFAULT_LOG "\"\n\n"
                        "Options,\n"
                        "\t--version,   -v\t Print the package version and exit.\n"
                        "\t--no-daemon, -n\t Do not attempt to daemonize the process\n"
                        "\t--log-level  -l\t The log severity [DEBUG -> INFO -> WARN -> FAIL]\n"
                        "\t--device,    -d\t The device file for the GPRS/GSM module, defaults to \"" DEFAULT_DEVICE "\"\n" 
                        "\t--help,      -h\t Print this message and exit\n" 
                        "Exit status,\n"
                        "\t" STR(NO_ERROR)    ", Normal operation\n"
                        "\t" STR(MINOR_ERROR) ", Minor error\n"
                        "\t" STR(MAJOR_ERROR) ", Major Error\n"
              );
}

static void mkdaemon()
{
        pid_t pid;

        pid = fork();
        if(pid < 0) {
                exit(MAJOR_ERROR);
        }

        if(pid > 0) {
                exit(NO_ERROR);
        }

        if(setsid() < 0) {
                exit(MAJOR_ERROR);
        }

        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        pid = fork();
        if(pid < 0) {
                exit(MAJOR_ERROR);
        }

        if(pid > 0) {
                exit(NO_ERROR);
        }

        umask(0);
        chdir("/");

        /* 
         * Attempt to close fds from proc fs
         * failing that, iterate from lowest to highest
         * fd attempting to close.
         */
        DIR *fds_dir;
        fds_dir = opendir("/proc/self/fd");
        if (fds_dir == NULL)
        {
                long max_fd;

                max_fd = sysconf(_SC_OPEN_MAX);
                if (max_fd < 0)
                        exit(MAJOR_ERROR);

                for (long i = 0; i < max_fd; i++)
                        close(i);
        }
        else
        {
                struct dirent *cur;
                int fds_fd = dirfd(fds_dir);

                errno = 0; 
                cur = readdir(fds_dir);
                for (; cur != NULL; cur = readdir(fds_dir))
                {
                        int cur_fd;

                        if (errno != 0)
                                exit(MAJOR_ERROR);
                        if (!(cur->d_type & DT_LNK))
                                continue;

                        cur_fd = strtol(cur->d_name, NULL, 10);
                        if (cur_fd == fds_fd)
                                continue;
                        close(cur_fd);
                        errno = 0; 
                }
                closedir(fds_dir);
        }
}
