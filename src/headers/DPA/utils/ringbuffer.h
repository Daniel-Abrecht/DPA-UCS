#ifndef DPA_RINGBUFFER_H
#define DPA_RINGBUFFER_H

#include <stddef.h>
#include <stdbool.h>

#define DPA_TYPED_RINGBUFFER_MEMBER_TEMPLATE(T) \
  const size_t size; \
  DPA_buffer_range_t range; \
  bool inverse; \
  const size_t type_size; \
  T*const buffer;

#define DPA_RINGBUFFER_TEMPLATE(T,NAME) \
  typedef union NAME { \
    DPA_ringbuffer_base_t super; \
    struct { \
      DPA_TYPED_RINGBUFFER_MEMBER_TEMPLATE(T) \
    }; \
  } NAME ## _t

#define DPAUCS_DEFINE_RINGBUFFER(B,T,name,S,I) \
  static B name ## _buffer[S]; \
  T name = { \
    .super = { \
      .size = S, \
      .range = { \
        .offset = I ? S : 0, \
        .size = 0 \
      }, \
      .inverse = I, \
      .type_size = sizeof( B ), \
      .buffer = name ## _buffer \
    } \
  }

#define DPA_RINGBUFFER_GET(buf) (buf)->buffer[ DPA_ringbuffer_increment_read(&(buf)->super) ]
#define DPA_RINGBUFFER_PUT(buf,x) ( ( DPA_ringbuffer_full(&(buf)->super) ) \
    ? false : ( ( (buf)->buffer[ DPA_ringbuffer_increment_write(&(buf)->super) ] = (x) ), true ) \
  )

#define DPA_ringbuffer_eof(buf) ( !(buf)->range.size )
#define DPA_ringbuffer_size(buf) ( (buf)->range.size )
#define DPA_ringbuffer_full(buf) ( (buf)->range.size == (buf)->size )
#define DPA_ringbuffer_begin(buf) (&(buf)->buffer[ (buf)->range.offset - (buf)->inverse ])
#define DPA_ringbuffer_remaining(buf) ( (buf)->size - (buf)->range.size )

typedef struct DPA_buffer_range {
  size_t offset;
  size_t size;
} DPA_buffer_range_t;

typedef struct DPA_ringbuffer_base {
  DPA_TYPED_RINGBUFFER_MEMBER_TEMPLATE(void)
} DPA_ringbuffer_base_t;

void DPA_ringbuffer_reset( DPA_ringbuffer_base_t* ringbuffer );
size_t DPA_ringbuffer_copy( DPA_ringbuffer_base_t* destination, DPA_ringbuffer_base_t* source, size_t size );
size_t DPA_ringbuffer_read( DPA_ringbuffer_base_t* ringbuffer, void* destination, size_t size );
size_t DPA_ringbuffer_write( DPA_ringbuffer_base_t* ringbuffer, const void* source, size_t size );
void DPA_ringbuffer_reverse( DPA_ringbuffer_base_t* ringbuffer );
size_t DPA_ringbuffer_skip_read( DPA_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPA_ringbuffer_skip_write( DPA_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPA_ringbuffer_rewind_read( DPA_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPA_ringbuffer_rewind_write( DPA_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPA_ringbuffer_increment_read( DPA_ringbuffer_base_t* ringbuffer ); // post increment
size_t DPA_ringbuffer_increment_write( DPA_ringbuffer_base_t* ringbuffer ); // post increment
size_t DPA_ringbuffer_decrement_read( DPA_ringbuffer_base_t* ringbuffer ); // pre decrement
size_t DPA_ringbuffer_decrement_write( DPA_ringbuffer_base_t* ringbuffer ); // post decrement

DPA_RINGBUFFER_TEMPLATE( char, DPA_char_ringbuffer );
DPA_RINGBUFFER_TEMPLATE( unsigned char, DPA_uchar_ringbuffer );
DPA_RINGBUFFER_TEMPLATE( struct DPA_bufferInfo, DPA_buffer_ringbuffer );

#endif