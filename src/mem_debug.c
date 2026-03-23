/*
 * mem_debug.c
 * Responsibility: Utility debug routines shared by transport and pipeline.
 */

#include "mem_debug.h"

#include <stdio.h>

#include "printf.h"

#include "transport_if.h"

void mem_debug_dump_bytes(
    const char *label,
    const uint8_t *data,
    size_t data_size,
    size_t max_dump_size)
{
  char line[96];
  size_t dump_size = (data_size < max_dump_size) ? data_size : max_dump_size;
  size_t i;
  size_t pos = 0u;

  snprintf(line, sizeof(line), "PFOLLOW: %s (%u bytes):\n", label, (unsigned int)dump_size);
  transport_if_console_write(line);
  for (i = 0; i < dump_size; i++) {
    if ((i % 16u) == 0u) {
      pos = (size_t)snprintf(line, sizeof(line), "PFOLLOW: %04u: ", (unsigned int)i);
    }
    if (pos < sizeof(line)) {
      pos += (size_t)snprintf(&line[pos], sizeof(line) - pos, "%02x ", data[i]);
    }
    if ((i % 16u) == 15u || i == (dump_size - 1u)) {
      if (pos < sizeof(line)) {
        snprintf(&line[pos], sizeof(line) - pos, "\n");
      } else {
        line[sizeof(line) - 2u] = '\n';
        line[sizeof(line) - 1u] = '\0';
      }
      transport_if_console_write(line);
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
