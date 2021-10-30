#ifndef LOG_H
#define LOG_H 1

typedef enum {
       DEBUG = 0,
       INFO = 1,
       WARN = 2,
       FAIL = 3,
} loglevel;

#define LOG_DBUG(...) do { _log_call_site(__FILE__, __LINE__, __func__); \
                                _log_to_fd(DEBUG, ##__VA_ARGS__);}while(0)
#define LOG_INFO(...) _log_to_fd(INFO, ##__VA_ARGS__)
#define LOG_WARN(...) _log_to_fd(WARN, ##__VA_ARGS__)
#define LOG_FAIL(...) _log_to_fd(FAIL, ##__VA_ARGS__)

loglevel ctoll(char *);
int log_init(int, loglevel);
int _log_call_site(const char*, const int, const char*);
int _log_to_fd(loglevel, char*, ...);

#endif
