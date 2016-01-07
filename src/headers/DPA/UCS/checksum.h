#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

struct DPAUCS_stream;

uint16_t checksum( void*, size_t );
uint16_t checksumOfStream( struct DPAUCS_stream*, size_t );

#endif