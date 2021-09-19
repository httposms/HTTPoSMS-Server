/* Feature test macros for dprintf() */
#define _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "log.h"

static int logfd;
static char **currprompt;
static loglevel level = DEBUG;

static char *ncolor[] = {
        "[ DBUG ] ",
        "[ INFO ] ",
        "[ WARN ] ",
        "[FAILED] "
};
static char *color[] = {
        "[ DBUG ] ",
        "[ \x1b[32mINFO\x1b[0m ] ",
        "[ \x1b[33mWARN\x1b[0m ] ",
        "[\x1b[31mFAILED\x1b[0m] "
};

loglevel ctoll(char *level)
{
        if(!strcmp(level, "DEBUG"))
                return DEBUG;
        if(!strcmp(level, "INFO"))
                return INFO;
        if(!strcmp(level, "WARN"))
                return WARN;
        if(!strcmp(level, "FAIL"))
                return FAIL;
        return DEBUG;
}

int log_init(int fd, loglevel severity)
{
        logfd = fd;
        level = severity;

        /* 
         * Is this a controlling terminal that we
         * we should print colored text to
         */
        if(isatty(fd)){
                currprompt = color;
        }
        else{  
                currprompt = ncolor;
        }
        return 0;
}

int _log_call_site(const char *file, const int line, const char *func)
{
        dprintf(logfd, "[ %s:%d:%s() ]", file, line, func);
        return 0;
}
int _log_to_fd(loglevel severity, char *fmt, ...)
{
        va_list args;
        if (severity < level)
                return 0;
        dprintf(logfd, currprompt[severity]);
        
        va_start(args, fmt);
        vdprintf(logfd, fmt, args);
        va_end(args);

        return 0;
}
