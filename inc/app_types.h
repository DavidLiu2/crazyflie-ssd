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
  uint32_t capture_us;
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

typedef struct {
  int32_t x_raw;
  int32_t scale_raw;
  int32_t visibility_raw;
  float x_error;
  float scale_proxy;
  float visibility_logit;
  float confidence;
  uint8_t visible;
} follow_result_t;

typedef struct {
  uint32_t frame_id;
  uint32_t capture_us;
  uint32_t preprocess_us;
  uint32_t inference_us;
  uint32_t postprocess_us;
  uint32_t total_us;
  uint16_t output_checksum;
  uint8_t app_packet_sent;
  follow_result_t follow;
} pipeline_result_t;

#endif /* APP_TYPES_H */
