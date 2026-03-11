/*
 * ssd_postprocess.h
 * Responsibility: Isolated SSD output postprocessing interface (stub for now).
 */

#ifndef SSD_POSTPROCESS_H
#define SSD_POSTPROCESS_H

#include <stddef.h>
#include <stdint.h>

#include "app_types.h"

size_t ssd_postprocess_run(
    const uint8_t *net_out,
    size_t net_out_size,
    person_det_t *out_dets,
    size_t max_dets);

#endif /* SSD_POSTPROCESS_H */