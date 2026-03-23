/*
 * follow_output.c
 * Responsibility: Decode the hybrid-follow head into useful runtime metrics.
 */

#include "follow_output.h"

#include <math.h>
#include <string.h>

#include "app_config.h"

static float clampf(float value, float min_value, float max_value)
{
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static float sigmoidf_safe(float value)
{
  if (value >= 0.0f) {
    float z = expf(-value);
    return 1.0f / (1.0f + z);
  }

  {
    float z = expf(value);
    return z / (1.0f + z);
  }
}

int follow_output_from_buffer(
    const uint8_t *net_out,
    size_t net_out_size,
    follow_result_t *result)
{
  int32_t raw_values[3];
  float decoded_x;
  float decoded_scale;
  float decoded_logit;

  if (net_out == NULL || result == NULL || net_out_size < sizeof(raw_values)) {
    return -1;
  }

  memset(result, 0, sizeof(*result));
  memcpy(raw_values, net_out, sizeof(raw_values));

  decoded_x = ((float)raw_values[0]) / APP_FOLLOW_OUTPUT_Q_SCALE;
  decoded_scale = ((float)raw_values[1]) / APP_FOLLOW_OUTPUT_Q_SCALE;
  decoded_logit = ((float)raw_values[2]) / APP_FOLLOW_OUTPUT_Q_SCALE;

  result->x_raw = raw_values[0];
  result->scale_raw = raw_values[1];
  result->visibility_raw = raw_values[2];
  result->x_error = clampf(decoded_x, -1.0f, 1.0f);
  result->scale_proxy = clampf(decoded_scale, 0.0f, 1.0f);
  result->visibility_logit = decoded_logit;
  result->confidence = sigmoidf_safe(decoded_logit);
  result->visible = (uint8_t)(result->confidence >= APP_FOLLOW_VISIBLE_CONFIDENCE_THRESHOLD);

  return 0;
}
