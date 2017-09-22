#ifndef DPA_UTILS_H
#define DPA_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <DPA/utils/helper_macros.h>

#define DPA_MIN( a, b ) (((a)<(b))?(a):(b))
#define DPA_MAX( a, b ) (((a)>(b))?(a):(b))

#define DPA_htob16 DPA_btoh16
#define DPA_htob32 DPA_btoh32

struct linked_data_list;
typedef struct linked_data_list {
  size_t size;
  const void* data;
  const struct linked_data_list* next;
} linked_data_list_t;

uint16_t DPA_btoh16( uint16_t ); // big endian to host byte order
uint32_t DPA_btoh32( uint32_t ); // big endian to host byte order

size_t DPA_progmem_strlen( const flash char* str );
void DPA_progmem_memcpy( void* dst, const flash void* src, size_t size );
bool DPA_mempos( size_t* position, const void* haystack, size_t haystack_size, const void* needle, size_t needle_size );
bool DPA_memrcharpos( size_t* position, size_t size, void* haystack, char needle );
void DPA_memtrim( const char**restrict mem, size_t*restrict size, char c );
bool DPA_streq_nocase( const char* str, const char* mem, size_t size );
bool DPA_streq_nocase_fn( const flash char* str, const char* mem, size_t size );
bool DPA_streq_nocase_ff( const flash char* str, const flash char* mem, size_t size );
void DPA_memrcpy( size_t size, void*restrict dest, const void*restrict src_end, size_t count );

#endif
