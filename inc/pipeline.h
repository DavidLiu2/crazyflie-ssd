/*
 * pipeline.h
 * Responsibility: End-to-end frame pipeline orchestration.
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include "app_types.h"

void pipeline_process_frame(const camera_frame_t *frame);

#endif /* PIPELINE_H */