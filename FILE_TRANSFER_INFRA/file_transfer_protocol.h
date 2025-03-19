#ifndef PROTOCOL_H   // If PROTOCOL_H is not defined
#define PROTOCOL_H   // Define PROTOCOL_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint32_t flags;  // 4 bytes (naturally aligned)
    size_t offset;   // 4 bytes (on 32-bit) or 8 bytes (on 64-bit)
    uint8_t data[];  // Flexible array
} data_chunk_protocol;

#endif // PROTOCOL_H