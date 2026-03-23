/*
 * net_runner.c
 * Responsibility: Own network runtime init, buffers, and inference execution.
 */

#include "net_runner.h"

#include <stddef.h>
#include <stdint.h>

#include "pmsis.h"

#include "app_config.h"
#include "app_log.h"
#include "mem.h"
#include "network.h"

static uint8_t *g_l2_arena = NULL;
static size_t g_l2_arena_bytes = 0;
static uint8_t *g_net_out = NULL;

static const size_t k_arena_candidates[] = {
  412000u,
  393216u,
  384000u,
  368640u,
  360448u,
  352256u,
  327680u
};

#if (APP_L2_PROBE_STEP_BYTES == 0)
#error "APP_L2_PROBE_STEP_BYTES must be > 0"
#endif

static size_t probe_max_l2_block(void)
{
  size_t size = (size_t)APP_L2_PROBE_START_BYTES;

  while (size >= (size_t)APP_L2_PROBE_MIN_BYTES) {
    void *block = pi_l2_malloc(size);
    if (block != NULL) {
      pi_l2_free(block, size);
      return size;
    }

    if (size <= (size_t)APP_L2_PROBE_STEP_BYTES) {
      break;
    }
    size -= (size_t)APP_L2_PROBE_STEP_BYTES;
  }

  return 0;
}

static int allocate_arena_with_candidates(void)
{
  size_t i;

  for (i = 0; i < (sizeof(k_arena_candidates) / sizeof(k_arena_candidates[0])); i++) {
    const size_t candidate = k_arena_candidates[i];
    g_l2_arena = (uint8_t *)pi_l2_malloc(candidate);
    app_log_debug(
        "arena candidate %u bytes: %s",
        (unsigned int)candidate,
        g_l2_arena ? "OK" : "FAILED");
    if (g_l2_arena != NULL) {
      g_l2_arena_bytes = candidate;
      app_log_debug(
          "selected L2 arena @ 0x%08lx (%u bytes)",
          (unsigned long)g_l2_arena,
          (unsigned int)g_l2_arena_bytes);
      return 0;
    }
  }

  app_log_error("L2 arena allocation failed for all candidates");
  return -1;
}

static int allocate_output_buffer(void)
{
  g_net_out = (uint8_t *)pi_l2_malloc(APP_NET_OUTPUT_BYTES);
  if (g_net_out == NULL) {
    app_log_error("network output allocation failed");
    if (g_l2_arena != NULL) {
      pi_l2_free(g_l2_arena, g_l2_arena_bytes);
      g_l2_arena = NULL;
      g_l2_arena_bytes = 0;
    }
    return -1;
  }
  app_log_debug(
      "net output @ 0x%08lx (%u bytes)",
      (unsigned long)g_net_out,
      (unsigned int)APP_NET_OUTPUT_BYTES);

  return 0;
}

static int allocate_runtime_buffers(void)
{
  if (allocate_arena_with_candidates()) {
    return -1;
  }
  return allocate_output_buffer();
}

int net_runner_init(void)
{
  app_log_debug(
      "net init arena order=%s requested_arena=%u output_bytes=%u",
      APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT ?
          "before_network_init" :
          "after_network_init",
      (unsigned int)APP_NET_ARENA_BYTES,
      (unsigned int)APP_NET_OUTPUT_BYTES);

  mem_init();
  app_log_debug(
      "max contiguous L2 block after mem_init=%u bytes",
      (unsigned int)probe_max_l2_block());

  if (APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT) {
    if (allocate_runtime_buffers()) {
      return -1;
    }
  }

  network_initialize();
  app_log_debug(
      "max contiguous L2 block after network_initialize=%u bytes",
      (unsigned int)probe_max_l2_block());

  if (!APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT) {
    if (allocate_runtime_buffers()) {
      return -1;
    }
  }

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
    app_log_error("network buffers are not initialized");
    return -1;
  }

  network_run(g_l2_arena, g_l2_arena_bytes, g_net_out, 0, 1);
  return 0;
}
