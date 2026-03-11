/*
 * app_config.h
 * Responsibility: Centralized compile-time configuration for the modular app.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define APP_VERBOSE 1

#define APP_SOC_FC_FREQ_HZ (100000000)
#define APP_SOC_CL_FREQ_HZ (100000000)

#define APP_NET_ARENA_BYTES (412000u)
#define APP_NET_OUTPUT_BYTES (1600u)
#define APP_NET_INPUT_W (160)
#define APP_NET_INPUT_H (160)
#ifndef APP_NET_INPUT_C
#define APP_NET_INPUT_C (3)
#endif
#define APP_NET_INPUT_BYTES ((APP_NET_INPUT_W) * (APP_NET_INPUT_H) * (APP_NET_INPUT_C))

#define APP_CAMERA_WIDTH (324)
#define APP_CAMERA_HEIGHT (244)
#define APP_CAMERA_FRAME_BYTES ((APP_CAMERA_WIDTH) * (APP_CAMERA_HEIGHT))
#define APP_CAMERA_CAPTURE_TIMEOUT_US (500000u)
#define APP_CAMERA_IMG_ORIENTATION_REG (0x0101u)

#define APP_UART_BAUDRATE_BPS (115200)
#define APP_IO_DUMP_BYTES (64u)
#define APP_COMPACT_OUTPUT_BYTES (16u)
#define APP_PACKET_MAGIC (0xA5u)

#endif /* APP_CONFIG_H */