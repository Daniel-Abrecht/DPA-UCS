#ifndef DPAUCS_UTILS_H
#define DPAUCS_UTILS_H

#include <stddef.h>
#include <stdbool.h>

#define DPAUCS_MIN( a, b ) (((a)<(b))?(a):(b))
#define DPAUCS_MAX( a, b ) (((a)>(b))?(a):(b))

bool DPA_mempos( size_t* position, void* haystack, size_t haystack_size, void* needle, size_t needle_size );
bool DPA_memrcharpos( size_t* position, size_t size, void* haystack, char needle );
void DPA_memtrim( const char**restrict mem, size_t*restrict size, char c );
bool DPA_streq_nocase( const char* str, const char* mem, size_t size );
void DPA_memrcpy( size_t size, void*restrict dest, const void*restrict src_end, size_t count );

#endif
