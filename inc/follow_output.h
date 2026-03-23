/*
 * follow_output.h
 * Responsibility: Decode the 3-value hybrid-follow network head output.
 */

#ifndef FOLLOW_OUTPUT_H
#define FOLLOW_OUTPUT_H

#include <stddef.h>
#include <stdint.h>

#include "app_types.h"

int follow_output_from_buffer(
    const uint8_t *net_out,
    size_t net_out_size,
    follow_result_t *result);

#endif /* FOLLOW_OUTPUT_H */
