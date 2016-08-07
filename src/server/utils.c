#include <string.h>
#include <DPA/UCS/utils.h>

bool mempos( size_t* position, void* haystack, size_t haystack_size, void* needle, size_t needle_size ){

  if( needle_size == 0 )
    return false;

  if( needle_size > haystack_size )
    return false;

  size_t n = haystack_size - needle_size + 1;
  size_t i=0, j=0;

  while( i < needle_size && n--+i )
    if( ((char*)needle)[i++] != ((char*)haystack)[j++] )
      n-=i-1, j-=i-1, i=0;

  if( i < needle_size )
    return false;

  *position = j-i;

  return true;
}

bool memrcharpos( size_t* position, size_t size, void* haystack, char needle ){
  char* ch = (char*)haystack + size;
  while( size-- )
  if( *--ch == needle ){
    *position = size;
    return true;
  }
  return false;
}

void memtrim( const char**restrict mem, size_t*restrict size, char c ){
  while( *size && **mem == c )
    (*mem)++, (*size)--;
  while( (*size) && (*mem)[(*size)-1] == c )
    (*size)--;
}

bool streq_nocase( const char* str, const char* mem, size_t size ){
  while( size-- ){
    char a = *str++;
    char b = *mem++;
    if( ( ( a >= 'A' && a <= 'Z' ) ? a - 'A' + 'a' : a ) != ( ( b >= 'A' && b <= 'Z' ) ? b - 'A' + 'a' : b ) )
      return false;
  }
  return true;
}

void memrcpy( size_t size, void*restrict dest, const void*restrict src, size_t count ){
  char* d = dest;
  const char* s = src;
  s += size * count;
  while( count-- ){
    s -= size;
    memcpy(d,s,size);
    d += size;
  }
}
