/*
 * net_runner.h
 * Responsibility: Network memory/runtime lifecycle and inference execution API.
 */

#ifndef NET_RUNNER_H
#define NET_RUNNER_H

#include <stddef.h>
#include <stdint.h>

int net_runner_init(void);
uint8_t *net_runner_get_input_buffer(void);
const uint8_t *net_runner_get_output_buffer(void);
size_t net_runner_get_output_size(void);
int net_runner_run(void);

#endif /* NET_RUNNER_H */