/*
 * app_main.c
 * Responsibility: AI-deck startup, CPX-visible logging, and async runtime loop.
 */

#include "app_main.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "printf.h"

#include "pmsis.h"
#include "bsp/bsp.h"

#include "app_config.h"
#include "app_log.h"
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
static volatile uint32_t g_processed_frame_counter = 0;
static volatile uint32_t g_dropped_frame_counter = 0;
static volatile uint32_t g_camera_timeout_counter = 0;
static uint32_t g_capture_start_us = 0;
static uint32_t g_boot_us = 0;
static uint32_t g_last_summary_log_us = 0;
static uint32_t g_last_heartbeat_log_us = 0;
static pipeline_result_t g_last_result;
static bool g_have_last_result = false;

static void soc_init(void)
{
#ifndef TARGET_CHIP_FAMILY_GAP9
  __pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1000);
#else
  pi_pmu_voltage_set(PI_PMU_VOLTAGE_DOMAIN_CHIP, PI_PMU_VOLT_800);
#endif
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_FC, APP_SOC_FC_FREQ_HZ);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_CL, APP_SOC_CL_FREQ_HZ);
  pi_time_wait_us(10000);
}

static void camera_capture_done(void *arg)
{
  (void)arg;

  camera_if_stop(&g_camera);
  g_capture_in_flight = false;
  g_frame.capture_us = pi_time_get_us() - g_capture_start_us;

  if (g_inference_running || g_frame_ready) {
    g_dropped_frame_counter++;
    return;
  }

  g_frame_counter++;
  g_frame.frame_id = g_frame_counter;
  g_frame_ready = true;
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

static uint32_t to_milli(float value)
{
  if (value >= 0.0f) {
    return (uint32_t)(value * 1000.0f + 0.5f);
  }

  return (uint32_t)((-value) * 1000.0f + 0.5f);
}

static void log_summary(const pipeline_result_t *result)
{
#if APP_DEBUG
  uint32_t cap_ms_whole = result->capture_us / 1000u;
  uint32_t cap_ms_frac = result->capture_us % 1000u;
  uint32_t infer_ms_whole = result->inference_us / 1000u;
  uint32_t infer_ms_frac = result->inference_us % 1000u;
  uint32_t total_ms_whole = result->total_us / 1000u;
  uint32_t total_ms_frac = result->total_us % 1000u;
  uint32_t conf_milli = to_milli(result->follow.confidence);
  uint32_t x_milli = to_milli(result->follow.x_error);
  uint32_t scale_milli = to_milli(result->follow.scale_proxy);
  char x_sign = (result->follow.x_error < 0.0f) ? '-' : '+';

  app_log_debug(
      "frame=%lu cap=%lu.%03lums infer=%lu.%03lums total=%lu.%03lums vis=%u conf=%lu.%03lu x=%c%lu.%03lu scale=%lu.%03lu raw=[%ld,%ld,%ld] drop=%lu camerr=%lu pkt=%u cksum=0x%04x",
      (unsigned long)result->frame_id,
      (unsigned long)cap_ms_whole,
      (unsigned long)cap_ms_frac,
      (unsigned long)infer_ms_whole,
      (unsigned long)infer_ms_frac,
      (unsigned long)total_ms_whole,
      (unsigned long)total_ms_frac,
      (unsigned int)result->follow.visible,
      (unsigned long)(conf_milli / 1000u),
      (unsigned long)(conf_milli % 1000u),
      x_sign,
      (unsigned long)(x_milli / 1000u),
      (unsigned long)(x_milli % 1000u),
      (unsigned long)(scale_milli / 1000u),
      (unsigned long)(scale_milli % 1000u),
      (long)result->follow.x_raw,
      (long)result->follow.scale_raw,
      (long)result->follow.visibility_raw,
      (unsigned long)g_dropped_frame_counter,
      (unsigned long)g_camera_timeout_counter,
      (unsigned int)result->app_packet_sent,
      (unsigned int)result->output_checksum);
#else
  (void)result;
#endif
}

static void maybe_log_heartbeat(uint32_t now_us)
{
#if APP_DEBUG
  if ((now_us - g_last_heartbeat_log_us) < APP_LOG_HEARTBEAT_INTERVAL_US) {
    return;
  }
  if ((now_us - g_last_summary_log_us) < APP_LOG_HEARTBEAT_INTERVAL_US) {
    g_last_heartbeat_log_us = now_us;
    return;
  }

  g_last_heartbeat_log_us = now_us;

  if (g_have_last_result) {
    app_log_debug(
        "heartbeat uptime=%lus frames=%lu processed=%lu last=%lu drop=%lu camerr=%lu ready=%u infer=%u",
        (unsigned long)((now_us - g_boot_us) / 1000000u),
        (unsigned long)g_frame_counter,
        (unsigned long)g_processed_frame_counter,
        (unsigned long)g_last_result.frame_id,
        (unsigned long)g_dropped_frame_counter,
        (unsigned long)g_camera_timeout_counter,
        (unsigned int)g_frame_ready,
        (unsigned int)g_inference_running);
  } else {
    app_log_debug(
        "heartbeat uptime=%lus frames=%lu processed=%lu drop=%lu camerr=%lu ready=%u infer=%u",
        (unsigned long)((now_us - g_boot_us) / 1000000u),
        (unsigned long)g_frame_counter,
        (unsigned long)g_processed_frame_counter,
        (unsigned long)g_dropped_frame_counter,
        (unsigned long)g_camera_timeout_counter,
        (unsigned int)g_frame_ready,
        (unsigned int)g_inference_running);
  }
#else
  (void)now_us;
#endif
}

void app_main_task(void *arg)
{
  pipeline_result_t result;

  (void)arg;

  pi_bsp_init();
  soc_init();
  g_boot_us = pi_time_get_us();

  if (transport_if_init()) {
    printf("PFOLLOW: ERROR: CPX init failed\n");
    pmsis_exit(-1);
  }
  app_log_info("app start");
  app_log_info("CPX init OK");

  if (camera_if_init(&g_camera)) {
    pmsis_exit(-1);
  }
  app_log_info("camera init OK");

  if (net_runner_init()) {
    pmsis_exit(-1);
  }
  app_log_info("model init OK");

  if (camera_if_alloc_frame(&g_frame)) {
    pmsis_exit(-1);
  }
  app_log_info("buffers ready");
  app_log_info("entering inference loop");

  camera_capture_start_async();

  while (1) {
    if (g_frame_ready && !g_inference_running) {
      g_frame_ready = false;
      g_inference_running = true;
      if (pipeline_process_frame(&g_frame, &result) == 0) {
        g_processed_frame_counter++;
        g_last_result = result;
        g_have_last_result = true;
#if APP_DEBUG
        if (APP_LOG_SUMMARY_EVERY_N_FRAMES != 0u &&
            (result.frame_id % APP_LOG_SUMMARY_EVERY_N_FRAMES) == 0u) {
          log_summary(&result);
          g_last_summary_log_us = pi_time_get_us();
        }
#endif
      } else {
        app_log_error("pipeline failed on frame=%lu", (unsigned long)g_frame.frame_id);
      }
      g_inference_running = false;
    }

    if (!g_capture_in_flight && !g_frame_ready && !g_inference_running) {
      camera_capture_start_async();
    }

    if (g_capture_in_flight) {
      uint32_t now_us = pi_time_get_us();
      if ((now_us - g_capture_start_us) > APP_CAMERA_CAPTURE_TIMEOUT_US) {
        g_camera_timeout_counter++;
        app_log_error(
            "camera capture timeout, retrying count=%lu",
            (unsigned long)g_camera_timeout_counter);
        camera_if_stop(&g_camera);
        g_capture_in_flight = false;
      }
    }

    maybe_log_heartbeat(pi_time_get_us());
    pi_yield();
  }
}
