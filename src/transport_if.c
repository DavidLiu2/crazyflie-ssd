/*
 * transport_if.c
 * Responsibility: UART-based compact packet transport for debug output.
 */

#include "transport_if.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "pmsis.h"

#include "app_config.h"
#include "mem_debug.h"

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t frame_id;
  uint16_t checksum;
  uint8_t output_head[APP_COMPACT_OUTPUT_BYTES];
} compact_inference_packet_t;

static struct pi_device g_uart;

int transport_if_init(void)
{
  struct pi_uart_conf conf;

  pi_uart_conf_init(&conf);
  conf.baudrate_bps = APP_UART_BAUDRATE_BPS;
  conf.enable_tx = 1;
  conf.enable_rx = 0;

  pi_open_from_conf(&g_uart, &conf);
  if (pi_uart_open(&g_uart)) {
    printf("ERROR: uart_init failed\n");
    return -1;
  }

  printf("uart_init done\n");
  return 0;
}

void transport_if_send_compact_packet(
    uint32_t frame_id,
    const uint8_t *net_out,
    size_t net_out_size)
{
  compact_inference_packet_t pkt;
  size_t copy_size =
      (net_out_size < (size_t)APP_COMPACT_OUTPUT_BYTES)
          ? net_out_size
          : (size_t)APP_COMPACT_OUTPUT_BYTES;

  memset(&pkt, 0, sizeof(pkt));
  pkt.magic = (uint8_t)APP_PACKET_MAGIC;
  pkt.frame_id = (uint8_t)(frame_id & 0xFFu);
  memcpy(pkt.output_head, net_out, copy_size);
  pkt.checksum = mem_debug_checksum16(pkt.output_head, APP_COMPACT_OUTPUT_BYTES);

  pi_uart_write(&g_uart, &pkt, sizeof(pkt));
}