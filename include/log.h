#ifndef LOG_H
#define LOG_H 1

#include <pthread.h>

typedef enum {
       DEBUG = 0,
       INFO = 1,
       WARN = 2,
       FAIL = 3,
} loglevel;

/* 
 * if log headers are included, but functions not used
 * this shows as unused. Can ignore in most cases
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
extern pthread_mutex_t _log_lck;
#pragma GCC diagnostic pop

#define _LOCK_WRAPPER(OP) do {pthread_mutex_lock(&_log_lck); OP; pthread_mutex_unlock(&_log_lck);} while(0) 
#define LOG_DBUG(...) _LOCK_WRAPPER( do { _log_call_site(__FILE__, __LINE__, __func__); _log_to_fd(DEBUG, ##__VA_ARGS__);}while(0))
#define LOG_INFO(...) _LOCK_WRAPPER(_log_to_fd(INFO, ##__VA_ARGS__))
#define LOG_WARN(...) _LOCK_WRAPPER(_log_to_fd(WARN, ##__VA_ARGS__))
#define LOG_FAIL(...) _LOCK_WRAPPER(_log_to_fd(FAIL, ##__VA_ARGS__))


loglevel ctoll(char *);
int log_init(int, loglevel);
int _log_call_site(const char*, const int, const char*);
int _log_to_fd(loglevel, char*, ...);

#endif
