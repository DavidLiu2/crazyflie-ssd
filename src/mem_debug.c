/*
 * mem_debug.c
 * Responsibility: Utility debug routines shared by transport and pipeline.
 */

#include "mem_debug.h"

#include <stdio.h>

void mem_debug_dump_bytes(
    const char *label,
    const uint8_t *data,
    size_t data_size,
    size_t max_dump_size)
{
  size_t dump_size = (data_size < max_dump_size) ? data_size : max_dump_size;
  size_t i;

  printf("%s (%u bytes):\n", label, (unsigned int)dump_size);
  for (i = 0; i < dump_size; i++) {
    if ((i % 16u) == 0u) {
      printf("%04u: ", (unsigned int)i);
    }
    printf("%02x ", data[i]);
    if ((i % 16u) == 15u || i == (dump_size - 1u)) {
      printf("\n");
    }
  }
}

uint16_t mem_debug_checksum16(const uint8_t *data, size_t size)
{
  uint32_t sum = 0;
  size_t i;

  for (i = 0; i < size; i++) {
    sum += data[i];
  }
  return (uint16_t)(sum & 0xFFFFu);
}