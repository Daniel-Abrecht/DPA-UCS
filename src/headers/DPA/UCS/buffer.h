#include <stddef.h>

#define BUFFER_TEMPLATE(T) \
  typedef struct { \
    T*const buffer; \
    const size_t size; \
    size_t read_offset; \
    size_t write_offset; \
  }

#define DEFINE_BUFFER(B,T,name,size) \
  static B name ## _buffer[size]; \
  T name = { name ## _buffer, size, 0, 0 }

#define BUFFER_SIZE(buf) ( (buf)->write_offset - (buf)->read_offset )
#define BUFFER_EOF(buf) ( !BUFFER_SIZE(buf) )
#define BUFFER_BEGIN(buf) ( &(buf)->buffer[ (buf)->read_offset % (buf)->size ] )
#define BUFFER_END(buf) \
  ( &(buf)->buffer[ \
    ( (buf)->read_offset % (buf)->size ) > ( (buf)->write_offset % (buf)->size ) \
      ? (buf)->write_offset % (buf)->size \
      : (buf)->size \
  ] )
#define BUFFER_SKIP(buf,x) do { (buf)->read_offset += x; } while(0)
#define BUFFER_GET(buf) (buf)->buffer[ (buf)->read_offset++ % (buf)->size ]
#define BUFFER_AT(buf,offset) (buf)->buffer[ ( (buf)->read_offset + (offset) ) % (buf)->size ]
#define BUFFER_FULL(buf) ( BUFFER_SIZE(buf) >= (buf)->size )
#define BUFFER_PUT(buf,x) \
  do { \
    (buf)->buffer[ (buf)->write_offset % (buf)->size ] = x; \
    (buf)->write_offset++; \
  } while(0)

#define BUFF_MAX ( 1<<(sizeof(size_t)-1) )

#define BUFFER_BUFFER_USERDEFINED(buf) ( BUFFER_GET(buf).type >= BUFFER_TYPE_SIZE )
#define BUFFER_BUFFER_TYPE(buf) ( BUFFER_GET(buf).type - BUFFER_TYPE_SIZE )
#define BUFFER_BUFFER_SIZE(buf) ( BUFFER_GET(buf).size )
#define BUFFER_BUFFER_PTR(buf) ( BUFFER_GET(buf).ptr )

typedef enum {
  BUFFER_BUFFER,
  BUFFER_ARRAY,
  BUFFER_TYPE_SIZE
} buffer_type;

typedef struct {
  buffer_type type;
  size_t size;
  size_t offset;
  const void* ptr;
} bufferInfo_t;

BUFFER_TEMPLATE(char) char_buffer_t;
BUFFER_TEMPLATE(unsigned char) uchar_buffer_t;
BUFFER_TEMPLATE(bufferInfo_t) buffer_buffer_t;
