/*
 * net_runner.c
 * Responsibility: Own network runtime init, buffers, and inference execution.
 */

#include "net_runner.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "pmsis.h"

#include "app_config.h"
#include "mem.h"
#include "network.h"

static uint8_t *g_l2_arena = NULL;
static uint8_t *g_net_out = NULL;

int net_runner_init(void)
{
  printf("BEFORE mem_init\n");
  mem_init();
  printf("AFTER mem_init\n");

  printf("BEFORE network_init\n");
  network_initialize();
  printf("AFTER network_init\n");

  g_l2_arena = (uint8_t *)pi_l2_malloc(APP_NET_ARENA_BYTES);
  if (g_l2_arena == NULL) {
    printf("ERROR: L2 arena allocation failed\n");
    return -1;
  }
  printf("L2 arena @ 0x%08x (%u bytes)\n",
         (unsigned int)g_l2_arena,
         (unsigned int)APP_NET_ARENA_BYTES);

  g_net_out = (uint8_t *)pi_l2_malloc(APP_NET_OUTPUT_BYTES);
  if (g_net_out == NULL) {
    printf("ERROR: net output allocation failed\n");
    return -1;
  }
  printf("Net output @ 0x%08x (%u bytes)\n",
         (unsigned int)g_net_out,
         (unsigned int)APP_NET_OUTPUT_BYTES);

  return 0;
}

uint8_t *net_runner_get_input_buffer(void)
{
  return g_l2_arena;
}

const uint8_t *net_runner_get_output_buffer(void)
{
  return g_net_out;
}

size_t net_runner_get_output_size(void)
{
  return (size_t)APP_NET_OUTPUT_BYTES;
}

int net_runner_run(void)
{
  if (g_l2_arena == NULL || g_net_out == NULL) {
    return -1;
  }

  printf("network run start\n");
  network_run(g_l2_arena, APP_NET_ARENA_BYTES, g_net_out, 0, 1);
  printf("network run done\n");

  return 0;
}