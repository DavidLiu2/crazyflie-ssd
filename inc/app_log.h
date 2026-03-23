/*
 * app_log.h
 * Responsibility: Compact CPX-console logging helpers for the flashed app.
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdint.h>

void app_log_info(const char *fmt, ...);
void app_log_error(const char *fmt, ...);
void app_log_debug(const char *fmt, ...);

#define app_log_every_n(counter, interval, ...)                      \
  do {                                                              \
    if ((interval) != 0u && ((counter) % (interval)) == 0u) {       \
      app_log_debug(__VA_ARGS__);                                   \
    }                                                               \
  } while (0)

#endif /* APP_LOG_H */
