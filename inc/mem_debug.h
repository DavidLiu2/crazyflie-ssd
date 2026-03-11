/*
 * mem_debug.h
 * Responsibility: Small debug helpers for byte dumps and checksums.
 */

#ifndef MEM_DEBUG_H
#define MEM_DEBUG_H

#include <stddef.h>
#include <stdint.h>

void mem_debug_dump_bytes(
    const char *label,
    const uint8_t *data,
    size_t data_size,
    size_t max_dump_size);

uint16_t mem_debug_checksum16(const uint8_t *data, size_t size);

#endif /* MEM_DEBUG_H */