#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#define NET_INFRA_LOG(level, format, ...) net_infra_log(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

void net_infra_log(log_level_t level, const char *file, int line, const char *func, const char *format, ...);

void net_infra_log_data(uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif