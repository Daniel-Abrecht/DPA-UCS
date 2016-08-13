#ifndef DPA_UTILS_H
#define DPA_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DPA_MIN( a, b ) (((a)<(b))?(a):(b))
#define DPA_MAX( a, b ) (((a)>(b))?(a):(b))

#define DPA_htob16 DPA_btoh16
#define DPA_htob32 DPA_btoh32

uint16_t DPA_btoh16( uint16_t ); // big endian to host byte order
uint32_t DPA_btoh32( uint32_t ); // big endian to host byte order

bool DPA_mempos( size_t* position, void* haystack, size_t haystack_size, void* needle, size_t needle_size );
bool DPA_memrcharpos( size_t* position, size_t size, void* haystack, char needle );
void DPA_memtrim( const char**restrict mem, size_t*restrict size, char c );
bool DPA_streq_nocase( const char* str, const char* mem, size_t size );
void DPA_memrcpy( size_t size, void*restrict dest, const void*restrict src_end, size_t count );

#endif
