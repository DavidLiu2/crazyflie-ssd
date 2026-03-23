/*
 * cpx.h
 * Responsibility: Bitcraze CPX API used for CFclient console logging and app packets.
 */

#ifndef CPX_H
#define CPX_H

#include <stdbool.h>
#include <stdint.h>

#include "com.h"

#define CPX_VERSION (0)
#define CPX_HEADER_SIZE (2)

typedef enum {
  CPX_T_STM32 = 1,
  CPX_T_ESP32 = 2,
  CPX_T_WIFI_HOST = 3,
  CPX_T_GAP8 = 4
} CPXTarget_t;

typedef enum {
  CPX_F_SYSTEM = 1,
  CPX_F_CONSOLE = 2,
  CPX_F_CRTP = 3,
  CPX_F_WIFI_CTRL = 4,
  CPX_F_APP = 5,
  CPX_F_TEST = 0x0E,
  CPX_F_BOOTLOADER = 0x0F,
  CPX_F_LAST
} CPXFunction_t;

typedef struct {
  CPXTarget_t destination;
  CPXTarget_t source;
  bool lastPacket;
  CPXFunction_t function;
  uint8_t version;
} CPXRouting_t;

typedef struct {
  CPXRouting_t route;
  uint16_t dataLength;
  uint8_t data[MTU - CPX_HEADER_SIZE];
} CPXPacket_t;

typedef enum {
  LOG_TO_WIFI = CPX_T_WIFI_HOST,
  LOG_TO_CRTP = CPX_T_STM32
} CPXConsoleTarget_t;

void cpxInit(void);
void cpxEnableFunction(CPXFunction_t function);
void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t *packet);
void cpxSendPacketBlocking(const CPXPacket_t *packet);
bool cpxSendPacket(const CPXPacket_t *packet, uint32_t timeout);
void cpxInitRoute(
    const CPXTarget_t source,
    const CPXTarget_t destination,
    const CPXFunction_t function,
    CPXRouting_t *route);
void cpxPrintToConsole(CPXConsoleTarget_t target, const char *fmt, ...);

#endif /* CPX_H */
