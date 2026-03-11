/*
 * preprocess.h
 * Responsibility: Camera frame preprocessing into network input tensor format.
 */

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <stdint.h>

void preprocess_camera_to_net_input(
    const uint8_t *cam_buf,
    int cam_w,
    int cam_h,
    uint8_t *net_in);

#endif /* PREPROCESS_H */