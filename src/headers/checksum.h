#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <stddef.h>
#include <stream.h>

uint16_t checksum( void*, size_t );
uint16_t checksumOfStream( DPAUCS_stream_t*, size_t );

#endif