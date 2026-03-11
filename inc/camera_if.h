/*
 * camera_if.h
 * Responsibility: Camera setup, frame-buffer allocation, and async capture API.
 */

#ifndef CAMERA_IF_H
#define CAMERA_IF_H

#include "pmsis.h"
#include "app_types.h"

int camera_if_init(struct pi_device *camera);
int camera_if_alloc_frame(camera_frame_t *frame);
void camera_if_start_capture_async(
    struct pi_device *camera,
    camera_frame_t *frame,
    pi_task_t *task,
    void (*cb)(void *),
    void *arg);
void camera_if_stop(struct pi_device *camera);

#endif /* CAMERA_IF_H */