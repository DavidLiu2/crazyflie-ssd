/*
 * preprocess.c
 * Responsibility: Crop/resize grayscale camera frame into network input tensor.
 */

#include "preprocess.h"

#include <stddef.h>

#include "app_config.h"

#if (APP_NET_INPUT_C != 1) && (APP_NET_INPUT_C != 3)
#error "APP_NET_INPUT_C must be 1 or 3"
#endif

void preprocess_camera_to_net_input(
    const uint8_t *cam_buf,
    int cam_w,
    int cam_h,
    uint8_t *net_in)
{
  int crop_size = (cam_w < cam_h) ? cam_w : cam_h;
  int crop_x = (cam_w - crop_size) / 2;
  int crop_y = (cam_h - crop_size) / 2;
  int y;
  int x;

  for (y = 0; y < APP_NET_INPUT_H; y++) {
    int src_y = crop_y + ((y * crop_size) / APP_NET_INPUT_H);
    const uint8_t *src_row = cam_buf + (src_y * cam_w);

    for (x = 0; x < APP_NET_INPUT_W; x++) {
      int src_x = crop_x + ((x * crop_size) / APP_NET_INPUT_W);
      uint8_t gray = src_row[src_x];

#if (APP_NET_INPUT_C == 1)
      size_t dst_idx = ((size_t)y * (size_t)APP_NET_INPUT_W) + (size_t)x;
      net_in[dst_idx] = gray;
#else
      size_t dst_idx =
          (((size_t)y * (size_t)APP_NET_INPUT_W) + (size_t)x) * (size_t)APP_NET_INPUT_C;
      net_in[dst_idx + 0u] = gray;
      net_in[dst_idx + 1u] = gray;
      net_in[dst_idx + 2u] = gray;
#endif
    }
  }
}