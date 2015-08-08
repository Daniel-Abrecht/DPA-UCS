#include <utils.h>

bool mempos( size_t* position, void* haystack, size_t haystack_size, void* needle, size_t needle_size ){

  if( needle_size == 0 ){
    *position = 0;
    return true;
  }

  if( needle_size > haystack_size )
    return false;

  size_t m = haystack_size - needle_size + 1;
  size_t i=0, j=0;

  while( i < needle_size && m-- )
    if( ((char*)needle)[i++] != ((char*)haystack)[j++] )
      j-=i-1, i=0;

  if( i != needle_size )
    return false;

  *position = j;

  return true;
}

bool memrcharpos( size_t* position, size_t size, void* haystack, char needle ){
  char* ch = (char*)haystack + size;
  size_t i = size;
  while( i-- && *--ch != needle );
  if(!~i) return false;
  *position = i;
  return true;
}
