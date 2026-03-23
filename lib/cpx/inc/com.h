/*
 * com.h
 * Responsibility: Low-level SPI transport API used by the Bitcraze CPX layer.
 */

#ifndef CPX_COM_H
#define CPX_COM_H

#include <stdint.h>

#define MTU (1022)

typedef struct {
  uint16_t len;
  uint8_t data[MTU];
} __attribute__((packed)) packet_t;

void com_init(void);
void com_read(packet_t *packet);
void com_write(packet_t *packet);

#endif /* CPX_COM_H */
