/*
 * transport_if.h
 * Responsibility: UART transport initialization and compact debug packet TX.
 */

#ifndef TRANSPORT_IF_H
#define TRANSPORT_IF_H

#include <stddef.h>
#include <stdint.h>

int transport_if_init(void);
void transport_if_send_compact_packet(
    uint32_t frame_id,
    const uint8_t *net_out,
    size_t net_out_size);

#endif /* TRANSPORT_IF_H */