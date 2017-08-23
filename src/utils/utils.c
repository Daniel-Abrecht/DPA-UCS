#include <string.h>
#include <DPA/utils/utils.h>

size_t DPA_progmem_strlen( const flash char* str ){
  size_t i = 0;
  while( *str++ )
    if( !++i )
      return ~0;
  return i;
}

void DPA_progmem_memcpy( void* dst, const flash void* src, size_t size ){
  while( size-- )
    ((char*)dst)[size] = ((const flash char*)src)[size];
}

bool DPA_mempos( size_t* position, const void* haystack, size_t haystack_size, const void* needle, size_t needle_size ){

  if( needle_size == 0 )
    return false;

  if( needle_size > haystack_size )
    return false;

  size_t n = haystack_size - needle_size + 1;
  size_t i=0, j=0;

  while( i < needle_size && n--+i )
    if( ((const char*)needle)[i++] != ((const char*)haystack)[j++] )
      n-=i-1, j-=i-1, i=0;

  if( i < needle_size )
    return false;

  *position = j-i;

  return true;
}

bool DPA_memrcharpos( size_t* position, size_t size, void* haystack, char needle ){
  char* ch = (char*)haystack + size;
  while( size-- )
  if( *--ch == needle ){
    *position = size;
    return true;
  }
  return false;
}

void DPA_memtrim( const char**restrict mem, size_t*restrict size, char c ){
  while( *size && **mem == c )
    (*mem)++, (*size)--;
  while( (*size) && (*mem)[(*size)-1] == c )
    (*size)--;
}

bool DPA_streq_nocase( const char* str, const char* mem, size_t size ){
  while( size-- ){
    char a = *str++;
    char b = *mem++;
    if( ( ( a >= 'A' && a <= 'Z' ) ? a - 'A' + 'a' : a ) != ( ( b >= 'A' && b <= 'Z' ) ? b - 'A' + 'a' : b ) )
      return false;
  }
  return true;
}

bool DPA_streq_nocase_fn( const flash char* str, const char* mem, size_t size ){
  while( size-- ){
    char a = *str++;
    char b = *mem++;
    if( ( ( a >= 'A' && a <= 'Z' ) ? a - 'A' + 'a' : a ) != ( ( b >= 'A' && b <= 'Z' ) ? b - 'A' + 'a' : b ) )
      return false;
  }
  return true;
}

bool DPA_streq_nocase_ff( const flash char* str, const flash char* mem, size_t size ){
  while( size-- ){
    char a = *str++;
    char b = *mem++;
    if( ( ( a >= 'A' && a <= 'Z' ) ? a - 'A' + 'a' : a ) != ( ( b >= 'A' && b <= 'Z' ) ? b - 'A' + 'a' : b ) )
      return false;
  }
  return true;
}

void DPA_memrcpy( size_t size, void*restrict dest, const void*restrict src, size_t count ){
  char* d = dest;
  const char* s = src;
  s += size * count;
  while( count-- ){
    s -= size;
    memcpy(d,s,size);
    d += size;
  }
}

uint16_t DPA_btoh16(uint16_t y){
  uint8_t* x = (uint8_t*)&y;
  return ( (uint16_t)x[0] << 8u )
       | ( (uint16_t)x[1] );
}

uint32_t DPA_btoh32(uint32_t y){
  uint8_t* x = (uint8_t*)&y;
  return ( (uint32_t)x[0] << 24u ) |
         ( (uint32_t)x[1] << 16u ) |
         ( (uint32_t)x[2] <<  8u ) |
         ( (uint32_t)x[3] );
}

