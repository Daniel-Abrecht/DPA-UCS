#ifndef DPAUCS_CHECKSUM_H
#define DPAUCS_CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

struct DPA_stream;

uint16_t checksum( const void*, size_t );
uint16_t checksumOfStream( struct DPA_stream*, size_t );

#endif
