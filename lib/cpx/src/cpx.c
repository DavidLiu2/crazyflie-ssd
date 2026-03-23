/*
 * cpx.c
 * Responsibility: Bitcraze CPX router and console helpers for AI-deck apps.
 */

#include "cpx.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "printf.h"

#include "pmsis.h"

typedef struct {
  CPXTarget_t destination : 3;
  CPXTarget_t source : 3;
  bool lastPacket : 1;
  bool reserved : 1;
  CPXFunction_t function : 6;
  uint8_t version : 2;
} __attribute__((packed)) CPXRoutingPacked_t;

typedef struct {
  uint16_t wireLength;
  CPXRoutingPacked_t route;
  uint8_t data[MTU - CPX_HEADER_SIZE];
} __attribute__((packed)) CPXPacketPacked_t;

#define QUEUE_LENGTH (2)

static xQueueHandle g_queues[CPX_F_LAST];
static SemaphoreHandle_t g_console_semaphore = NULL;
static CPXPacket_t g_rx_packet;
static CPXPacket_t g_console_tx_packet;
static CPXPacketPacked_t g_rx_packet_packed;
static CPXPacketPacked_t g_tx_packet_packed;

static void cpx_rx_task(void *parameters)
{
  (void)parameters;

  while (1) {
    uint16_t wire_length;

    com_read((packet_t *)&g_rx_packet_packed);

    configASSERT(CPX_VERSION == g_rx_packet_packed.route.version);
    wire_length = g_rx_packet_packed.wireLength;
    if (wire_length < CPX_HEADER_SIZE || wire_length > MTU) {
      printf("Dropping malformed CPX packet len=%u\n", (unsigned int)wire_length);
      continue;
    }

    g_rx_packet.route.version = g_rx_packet_packed.route.version;
    g_rx_packet.dataLength = wire_length - CPX_HEADER_SIZE;
    g_rx_packet.route.destination = g_rx_packet_packed.route.destination;
    g_rx_packet.route.source = g_rx_packet_packed.route.source;
    g_rx_packet.route.function = g_rx_packet_packed.route.function;
    g_rx_packet.route.lastPacket = g_rx_packet_packed.route.lastPacket;
    memcpy(g_rx_packet.data, g_rx_packet_packed.data, g_rx_packet.dataLength);

    if (g_rx_packet.route.function < CPX_F_LAST &&
        g_queues[g_rx_packet.route.function] != 0) {
      xQueueSend(g_queues[g_rx_packet.route.function], &g_rx_packet, portMAX_DELAY);
    } else {
      printf("No queue setup for function %d\n", g_rx_packet.route.function);
    }
  }
}

void cpxEnableFunction(CPXFunction_t function)
{
  configASSERT(function < CPX_F_LAST);
  g_queues[function] = xQueueCreate(QUEUE_LENGTH, sizeof(CPXPacket_t));
  configASSERT(g_queues[function] != 0);
}

void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t *packet)
{
  xQueueReceive(g_queues[function], packet, portMAX_DELAY);
}

void cpxSendPacketBlocking(const CPXPacket_t *packet)
{
  g_tx_packet_packed.wireLength = packet->dataLength + CPX_HEADER_SIZE;
  g_tx_packet_packed.route.destination = packet->route.destination;
  g_tx_packet_packed.route.source = packet->route.source;
  g_tx_packet_packed.route.function = packet->route.function;
  g_tx_packet_packed.route.version = packet->route.version;
  g_tx_packet_packed.route.lastPacket = packet->route.lastPacket;
  memcpy(g_tx_packet_packed.data, packet->data, packet->dataLength);
  com_write((packet_t *)&g_tx_packet_packed);
}

bool cpxSendPacket(const CPXPacket_t *packet, uint32_t timeout)
{
  (void)timeout;
  cpxSendPacketBlocking(packet);
  return true;
}

void cpxPrintToConsole(CPXConsoleTarget_t target, const char *fmt, ...)
{
  if (xSemaphoreTake(g_console_semaphore, portMAX_DELAY) == pdTRUE) {
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf((char *)g_console_tx_packet.data, sizeof(g_console_tx_packet.data), fmt, ap);
    va_end(ap);

    if (len < 0) {
      len = 0;
    }
    if (len >= (int)sizeof(g_console_tx_packet.data)) {
      len = (int)sizeof(g_console_tx_packet.data) - 1;
      g_console_tx_packet.data[len] = '\0';
    }

    g_console_tx_packet.route.destination = target;
    g_console_tx_packet.route.source = CPX_T_GAP8;
    g_console_tx_packet.route.function = CPX_F_CONSOLE;
    g_console_tx_packet.route.version = CPX_VERSION;
    g_console_tx_packet.route.lastPacket = true;
    g_console_tx_packet.dataLength = (uint16_t)(len + 1);

    cpxSendPacketBlocking(&g_console_tx_packet);
    xSemaphoreGive(g_console_semaphore);
  }
}

void cpxInitRoute(
    const CPXTarget_t source,
    const CPXTarget_t destination,
    const CPXFunction_t function,
    CPXRouting_t *route)
{
  route->source = source;
  route->destination = destination;
  route->function = function;
  route->version = CPX_VERSION;
  route->lastPacket = true;
}

void cpxInit(void)
{
  BaseType_t rx_task;

  com_init();

  memset(g_queues, 0, sizeof(g_queues));
  rx_task = xTaskCreate(
      cpx_rx_task,
      "rx_task",
      configMINIMAL_STACK_SIZE * 2,
      NULL,
      tskIDLE_PRIORITY + 1,
      NULL);

  if (rx_task != pdPASS) {
    printf("Could not start router rx tasks!\n");
    pmsis_exit(-1);
  }

  g_console_semaphore = xSemaphoreCreateBinary();
  configASSERT(g_console_semaphore != NULL);
  xSemaphoreGive(g_console_semaphore);
}
