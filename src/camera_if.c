/*
 * camera_if.c
 * Responsibility: Camera device init and frame capture plumbing.
 */

#include "camera_if.h"

#include <stdint.h>

#include "bsp/camera/himax.h"

#include "app_config.h"
#include "app_log.h"

int camera_if_init(struct pi_device *camera)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);
  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(camera, &cam_conf);
  if (pi_camera_open(camera)) {
    app_log_error("camera init failed");
    return -1;
  }

  /* Match Bitcraze examples: set orientation then initialize AEG. */
  pi_camera_control(camera, PI_CAMERA_CMD_START, 0);
  {
    uint8_t set_value = 3;
    uint8_t reg_value = 0;
    pi_camera_reg_set(camera, APP_CAMERA_IMG_ORIENTATION_REG, &set_value);
    pi_time_wait_us(1000000);
    pi_camera_reg_get(camera, APP_CAMERA_IMG_ORIENTATION_REG, &reg_value);
    if (set_value != reg_value) {
      app_log_info(
          "camera orientation register mismatch set=%u got=%u",
          (unsigned int)set_value,
          (unsigned int)reg_value);
    }
  }
  pi_camera_control(camera, PI_CAMERA_CMD_STOP, 0);
  pi_camera_control(camera, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}

int camera_if_alloc_frame(camera_frame_t *frame)
{
  frame->data = (uint8_t *)pi_l2_malloc(APP_CAMERA_FRAME_BYTES);
  if (frame->data == NULL) {
    app_log_error("camera frame buffer allocation failed");
    return -1;
  }

  frame->width = (uint16_t)APP_CAMERA_WIDTH;
  frame->height = (uint16_t)APP_CAMERA_HEIGHT;
  frame->bytes = APP_CAMERA_FRAME_BYTES;
  frame->frame_id = 0;
  frame->capture_us = 0;

  app_log_debug(
      "camera frame buffer @ 0x%08lx (%u bytes)",
      (unsigned long)frame->data,
      (unsigned int)frame->bytes);
  return 0;
}

void camera_if_start_capture_async(
    struct pi_device *camera,
    camera_frame_t *frame,
    pi_task_t *task,
    void (*cb)(void *),
    void *arg)
{
  pi_camera_capture_async(
      camera,
      frame->data,
      frame->bytes,
      pi_task_callback(task, cb, arg));
  pi_camera_control(camera, PI_CAMERA_CMD_START, 0);
}

void camera_if_stop(struct pi_device *camera)
{
  pi_camera_control(camera, PI_CAMERA_CMD_STOP, 0);
}
