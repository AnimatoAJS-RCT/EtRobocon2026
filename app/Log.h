#ifndef ETTR_APP_LOG_H_
#define ETTR_APP_LOG_H_

#include <stdio.h>

// Log level: 0=off, 1=error, 2=warn, 3=info, 4=debug
#ifndef ETTR_LOG_LEVEL
#define ETTR_LOG_LEVEL 4
#endif

#define ETTR_LOG_LEVEL_ERROR 1
#define ETTR_LOG_LEVEL_WARN  2
#define ETTR_LOG_LEVEL_INFO  3
#define ETTR_LOG_LEVEL_DEBUG 4

#define ETTR_LOG_CONCAT_INNER(a, b) a##b
#define ETTR_LOG_CONCAT(a, b) ETTR_LOG_CONCAT_INNER(a, b)

#define ETTR_LOG(level, fmt, ...)                                                     \
    do {                                                                              \
        if(ETTR_LOG_LEVEL >= (level)) {                                               \
            printf((fmt), ##__VA_ARGS__);                                             \
        }                                                                             \
    } while(0)

#define ETTR_LOG_EVERY(level, interval, fmt, ...)                                     \
    do {                                                                              \
        if(ETTR_LOG_LEVEL >= (level)) {                                               \
            static unsigned int ETTR_LOG_CONCAT(_ettr_log_counter_, __LINE__) = 0U;  \
            unsigned int *cnt = &ETTR_LOG_CONCAT(_ettr_log_counter_, __LINE__);       \
            if(((*cnt)++ % (unsigned int)(interval)) == 0U) {                         \
                printf((fmt), ##__VA_ARGS__);                                         \
            }                                                                         \
        }                                                                             \
    } while(0)

#define LOGE(fmt, ...) ETTR_LOG(ETTR_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ETTR_LOG(ETTR_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) ETTR_LOG(ETTR_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) ETTR_LOG(ETTR_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define LOGE_EVERY(interval, fmt, ...) ETTR_LOG_EVERY(ETTR_LOG_LEVEL_ERROR, interval, fmt, ##__VA_ARGS__)
#define LOGW_EVERY(interval, fmt, ...) ETTR_LOG_EVERY(ETTR_LOG_LEVEL_WARN, interval, fmt, ##__VA_ARGS__)
#define LOGI_EVERY(interval, fmt, ...) ETTR_LOG_EVERY(ETTR_LOG_LEVEL_INFO, interval, fmt, ##__VA_ARGS__)
#define LOGD_EVERY(interval, fmt, ...) ETTR_LOG_EVERY(ETTR_LOG_LEVEL_DEBUG, interval, fmt, ##__VA_ARGS__)

#endif  // ETTR_APP_LOG_H_
