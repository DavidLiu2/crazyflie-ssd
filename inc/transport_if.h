/*
 * transport_if.h
 * Responsibility: UART transport initialization and compact debug packet TX.
 */

#ifndef TRANSPORT_IF_H
#define TRANSPORT_IF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_types.h"

int transport_if_init(void);
bool transport_if_console_ready(void);
void transport_if_console_write(const char *message);
int transport_if_send_follow_result(
    uint32_t frame_id,
    const follow_result_t *result);

#endif /* TRANSPORT_IF_H */
