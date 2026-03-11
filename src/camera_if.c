/*
 * camera_if.c
 * Responsibility: Camera device init and frame capture plumbing.
 */

#include "camera_if.h"

#include <stdint.h>
#include <stdio.h>

#include "bsp/camera/himax.h"

#include "app_config.h"

int camera_if_init(struct pi_device *camera)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);
  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(camera, &cam_conf);
  if (pi_camera_open(camera)) {
    printf("ERROR: camera_init failed\n");
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
      printf("WARN: camera orientation register mismatch (set=%u got=%u)\n",
             set_value,
             reg_value);
    }
  }
  pi_camera_control(camera, PI_CAMERA_CMD_STOP, 0);
  pi_camera_control(camera, PI_CAMERA_CMD_AEG_INIT, 0);

  printf("camera_init done\n");
  return 0;
}

int camera_if_alloc_frame(camera_frame_t *frame)
{
  frame->data = (uint8_t *)pi_l2_malloc(APP_CAMERA_FRAME_BYTES);
  if (frame->data == NULL) {
    printf("ERROR: camera frame buffer allocation failed\n");
    return -1;
  }

  frame->width = (uint16_t)APP_CAMERA_WIDTH;
  frame->height = (uint16_t)APP_CAMERA_HEIGHT;
  frame->bytes = APP_CAMERA_FRAME_BYTES;
  frame->frame_id = 0;

  printf("Camera frame buffer @ 0x%08x (%u bytes)\n",
         (unsigned int)frame->data,
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