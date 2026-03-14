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
    printf("Arena candidate %u bytes: %s\n",
           (unsigned int)candidate,
           g_l2_arena ? "OK" : "FAILED");
    if (g_l2_arena != NULL) {
      g_l2_arena_bytes = candidate;
      printf("Selected arena candidate: %u bytes\n", (unsigned int)g_l2_arena_bytes);
      printf("L2 arena @ 0x%08x (%u bytes)\n",
             (unsigned int)g_l2_arena,
             (unsigned int)g_l2_arena_bytes);
      return 0;
    }
  }

  printf("ERROR: L2 arena allocation failed for all candidates\n");
  return -1;
}

static int allocate_output_buffer(void)
{
  g_net_out = (uint8_t *)pi_l2_malloc(APP_NET_OUTPUT_BYTES);
  if (g_net_out == NULL) {
    printf("ERROR: net output allocation failed\n");
    if (g_l2_arena != NULL) {
      pi_l2_free(g_l2_arena, g_l2_arena_bytes);
      g_l2_arena = NULL;
      g_l2_arena_bytes = 0;
    }
    return -1;
  }
  printf("Net output @ 0x%08x (%u bytes)\n",
         (unsigned int)g_net_out,
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
  const size_t generated_output_bytes = network_generated_output_bytes();
  const size_t generated_peak_l2_usage = network_generated_peak_l2_usage_bytes();
  const size_t generated_required_arena = network_generated_required_l2_arena_bytes();

  printf("Arena allocation order: %s\n",
         APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT ?
         "before network_initialize()" :
         "after network_initialize()");
  printf("Requested arena/output: APP_NET_ARENA_BYTES=%u APP_NET_OUTPUT_BYTES=%u\n",
         (unsigned int)APP_NET_ARENA_BYTES,
         (unsigned int)APP_NET_OUTPUT_BYTES);
  printf("Generated output bytes (activations_out_size[70]): %u\n",
         (unsigned int)generated_output_bytes);
  printf("Generated L2 peak usage (from network_run_cluster schedule): %u\n",
         (unsigned int)generated_peak_l2_usage);
  printf("Generated required L2 arena (strict '<' in directional allocator): %u\n",
         (unsigned int)generated_required_arena);

  printf("BEFORE mem_init\n");
  mem_init();
  printf("AFTER mem_init\n");
  printf("Max contiguous L2 block after mem_init: %u bytes\n",
         (unsigned int)probe_max_l2_block());

  if (APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT) {
    if (allocate_runtime_buffers()) {
      return -1;
    }
  }

  printf("BEFORE network_init\n");
  network_initialize();
  printf("AFTER network_init\n");
  printf("Max contiguous L2 block after network_initialize: %u bytes\n",
         (unsigned int)probe_max_l2_block());

  if (!APP_ALLOCATE_ARENA_BEFORE_NETWORK_INIT) {
    if (allocate_runtime_buffers()) {
      return -1;
    }
  }

  if (g_l2_arena_bytes < generated_required_arena) {
    printf("WARNING: selected arena (%u) < generated required (%u)\n",
           (unsigned int)g_l2_arena_bytes,
           (unsigned int)generated_required_arena);
  }
  if ((size_t)APP_NET_OUTPUT_BYTES != generated_output_bytes) {
    printf("WARNING: APP_NET_OUTPUT_BYTES (%u) != generated output (%u)\n",
           (unsigned int)APP_NET_OUTPUT_BYTES,
           (unsigned int)generated_output_bytes);
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
    return -1;
  }

  printf("network run start\n");
  network_run(g_l2_arena, g_l2_arena_bytes, g_net_out, 0, 1);
  printf("network run done\n");

  return 0;
}
