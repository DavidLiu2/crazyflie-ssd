/*
 * app_types.h
 * Responsibility: Shared application-level data types across modules.
 */

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

typedef struct {
  uint8_t *data;
  uint16_t width;
  uint16_t height;
  uint32_t bytes;
  uint32_t frame_id;
} camera_frame_t;

typedef struct {
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  uint16_t score_q10;
  uint8_t class_id;
  uint8_t valid;
} person_det_t;

#endif /* APP_TYPES_H */