/*
 * transport_if.c
 * Responsibility: CPX console logging plus a separate CPX app-packet path.
 */

#include "transport_if.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "printf.h"

#include "app_config.h"
#include "cpx.h"

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint16_t payload_len;
  uint32_t frame_id;
  int32_t x_raw;
  int32_t scale_raw;
  int32_t visibility_raw;
} follow_packet_t;

static bool g_console_ready = false;
static CPXPacket_t g_follow_packet;

int transport_if_init(void)
{
  cpxInit();
  cpxEnableFunction(CPX_F_APP);
  cpxInitRoute(CPX_T_GAP8, CPX_T_STM32, CPX_F_APP, &g_follow_packet.route);
  g_console_ready = true;
  return 0;
}

bool transport_if_console_ready(void)
{
  return g_console_ready;
}

void transport_if_console_write(const char *message)
{
  if (message == NULL) {
    return;
  }

  if (g_console_ready) {
    cpxPrintToConsole(LOG_TO_CRTP, "%s", message);
  } else {
    printf("%s", message);
  }
}

int transport_if_send_follow_result(
    uint32_t frame_id,
    const follow_result_t *result)
{
#if APP_ENABLE_CPX_APP_PACKET_TX
  follow_packet_t packet;

  if (!g_console_ready || result == NULL) {
    return -1;
  }

  packet.magic = (uint8_t)APP_PACKET_MAGIC;
  packet.version = (uint8_t)APP_PACKET_VERSION;
  packet.payload_len = (uint16_t)(sizeof(packet) - sizeof(packet.magic) - sizeof(packet.version) - sizeof(packet.payload_len));
  packet.frame_id = frame_id;
  packet.x_raw = result->x_raw;
  packet.scale_raw = result->scale_raw;
  packet.visibility_raw = result->visibility_raw;

  memcpy(g_follow_packet.data, &packet, sizeof(packet));
  g_follow_packet.dataLength = (uint16_t)sizeof(packet);
  cpxSendPacketBlocking(&g_follow_packet);
  return 0;
#else
  (void)frame_id;
  (void)result;
  return -1;
#endif
}
