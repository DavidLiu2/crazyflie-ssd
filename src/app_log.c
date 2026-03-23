/*
 * app_log.c
 * Responsibility: Small logging wrapper that routes visible logs over CPX.
 */

#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "printf.h"

#include "app_config.h"
#include "transport_if.h"

#define APP_LOG_MESSAGE_BYTES (192u)
#define APP_LOG_LINE_BYTES (240u)

static void app_log_vwrite(const char *level, const char *fmt, va_list args)
{
  char message[APP_LOG_MESSAGE_BYTES];
  char line[APP_LOG_LINE_BYTES];
  size_t len;
  int written;

  written = vsnprintf(message, sizeof(message), fmt, args);
  if (written < 0) {
    return;
  }

  if (level != NULL && level[0] != '\0') {
    written = snprintf(line, sizeof(line), "PFOLLOW: %s%s", level, message);
  } else {
    written = snprintf(line, sizeof(line), "PFOLLOW: %s", message);
  }
  if (written < 0) {
    return;
  }

  len = strlen(line);
  if (len == 0u || line[len - 1u] != '\n') {
    if (len + 1u < sizeof(line)) {
      line[len] = '\n';
      line[len + 1u] = '\0';
    } else {
      line[sizeof(line) - 2u] = '\n';
      line[sizeof(line) - 1u] = '\0';
    }
  }

  transport_if_console_write(line);
}

void app_log_info(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  app_log_vwrite("", fmt, args);
  va_end(args);
}

void app_log_error(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  app_log_vwrite("ERROR: ", fmt, args);
  va_end(args);
}

void app_log_debug(const char *fmt, ...)
{
#if APP_DEBUG
  va_list args;

  va_start(args, fmt);
  app_log_vwrite("", fmt, args);
  va_end(args);
#else
  (void)fmt;
#endif
}
