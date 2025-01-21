#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

void common_log(log_level_t level, const char *file, int line, const char *func, const char *format, ...);

#define NET_LOG(level, format, ...) common_log(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

void printData(uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif