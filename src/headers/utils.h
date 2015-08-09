#ifndef DPAUCS_UTILS_H
#define DPAUCS_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool mempos( size_t* position, void* haystack, size_t haystack_size, void* needle, size_t needle_size );
bool memrcharpos( size_t* position, size_t size, void* haystack, char needle );
void memtrim( const char**restrict mem, size_t*restrict size, char c );

#endif