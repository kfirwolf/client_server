#include <stdarg.h>
#include <stdlib.h>
#include "net_infra.h"

void common_log(log_level_t level, const char *file, int line, const char *func, const char *format, ...) {
    va_list args;
    va_start(args, format);

    const char *level_str;
    switch (level) {
        case LOG_INFO:    level_str = "INFO";    break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_ERROR:   level_str = "ERROR";   break;
        case LOG_DEBUG:   level_str = "DEBUG";   break;
        default:          level_str = "UNKNOWN"; break;
    }

    printf("[%s] [%s:%d - %s] ", level_str, file, line, func);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

void printData(uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf(" %d ", data[i]);
    }
    printf("\n");
}