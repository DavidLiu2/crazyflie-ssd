/*
 * ssd_postprocess.c
 * Responsibility: Isolate SSD decode/postprocess stage (stub implementation).
 */

#include "ssd_postprocess.h"

#include <stddef.h>
#include <string.h>

size_t ssd_postprocess_run(
    const uint8_t *net_out,
    size_t net_out_size,
    person_det_t *out_dets,
    size_t max_dets)
{
  (void)net_out;
  (void)net_out_size;

  if (out_dets != NULL && max_dets > 0u) {
    memset(out_dets, 0, max_dets * sizeof(person_det_t));
  }

  return 0u;
}