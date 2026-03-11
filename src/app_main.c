/*
 * app_main.c
 * Responsibility: App lifecycle, async frame loop, and module initialization.
 */

#include "app_main.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pmsis.h"
#include "bsp/bsp.h"

#include "app_config.h"
#include "app_types.h"
#include "camera_if.h"
#include "net_runner.h"
#include "pipeline.h"
#include "transport_if.h"

static struct pi_device g_camera;
static camera_frame_t g_frame;
static pi_task_t g_capture_task;

static volatile bool g_capture_in_flight = false;
static volatile bool g_frame_ready = false;
static volatile bool g_inference_running = false;
static volatile uint32_t g_frame_counter = 0;
static volatile uint32_t g_dropped_frame_counter = 0;
static uint32_t g_capture_start_us = 0;

static void soc_init(void)
{
#ifndef TARGET_CHIP_FAMILY_GAP9
  PMU_set_voltage(1000, 0);
#else
  pi_pmu_voltage_set(PI_PMU_VOLTAGE_DOMAIN_CHIP, PI_PMU_VOLT_800);
#endif
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_FC, APP_SOC_FC_FREQ_HZ);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_CL, APP_SOC_CL_FREQ_HZ);
  pi_time_wait_us(10000);
  printf("soc_init done\n");
}

static void cpx_init_stub(void)
{
  printf("cpx_init skipped (not wired in this app)\n");
}

static void cluster_init(void)
{
  struct pi_device cluster;
  struct pi_cluster_conf conf;

  pi_cluster_conf_init(&conf);
  conf.id = 0;

  pi_open_from_conf(&cluster, &conf);
  if (pi_cluster_open(&cluster)) {
    printf("ERROR: cluster_init failed\n");
    pmsis_exit(-1);
  }
  printf("cluster_init done\n");

  /* Keep DORY integration untouched: network_run() still opens/closes internally. */
  pi_cluster_close(&cluster);
}

static void trace_init_stub(void)
{
  printf("trace_init skipped (no trace backend in this app)\n");
}

static void cpx_start_stub(void)
{
  printf("cpx_start skipped (not wired in this app)\n");
}

static void streamer_start_stub(void)
{
  printf("streamer_start skipped (not wired in this app)\n");
}

static void camera_capture_done(void *arg)
{
  (void)arg;

  camera_if_stop(&g_camera);
  g_capture_in_flight = false;

  if (g_inference_running || g_frame_ready) {
    g_dropped_frame_counter++;
    printf("frame dropped (inference busy), total dropped: %lu\n",
           (unsigned long)g_dropped_frame_counter);
    return;
  }

  g_frame_counter++;
  g_frame.frame_id = g_frame_counter;
  g_frame_ready = true;
  printf("frame captured (%lu)\n", (unsigned long)g_frame_counter);
}

static void camera_capture_start_async(void)
{
  camera_if_start_capture_async(
      &g_camera,
      &g_frame,
      &g_capture_task,
      camera_capture_done,
      NULL);
  g_capture_start_us = pi_time_get_us();
  g_capture_in_flight = true;
}

void app_main_task(void *arg)
{
  (void)arg;

  pi_bsp_init();

  printf("MAIN START\n");

  soc_init();

  if (transport_if_init()) {
    pmsis_exit(-1);
  }

  if (camera_if_init(&g_camera)) {
    pmsis_exit(-1);
  }

  cpx_init_stub();
  cluster_init();

  if (net_runner_init()) {
    pmsis_exit(-1);
  }

  if (camera_if_alloc_frame(&g_frame)) {
    pmsis_exit(-1);
  }

  trace_init_stub();

  printf("*** Initialization done ***\n");

  camera_capture_start_async();
  cpx_start_stub();
  streamer_start_stub();

  while (1) {
    if (g_frame_ready && !g_inference_running) {
      g_frame_ready = false;
      g_inference_running = true;
      pipeline_process_frame(&g_frame);
      g_inference_running = false;
    }

    if (!g_capture_in_flight && !g_frame_ready && !g_inference_running) {
      camera_capture_start_async();
    }

    if (g_capture_in_flight) {
      uint32_t now_us = pi_time_get_us();
      if ((now_us - g_capture_start_us) > APP_CAMERA_CAPTURE_TIMEOUT_US) {
        printf("Failed camera acquisition, retrying\n");
        camera_if_stop(&g_camera);
        g_capture_in_flight = false;
      }
    }

    pi_yield();
  }
}