#include <stddef.h>
#include <stdbool.h>

#define DPAUCS_TYPED_RINGBUFFER_MEMBER_TEMPLATE(T) \
  const size_t size; \
  DPAUCS_buffer_range_t range; \
  bool inverse; \
  const size_t type_size; \
  T*const buffer;

#define DPAUCS_RINGBUFFER_TEMPLATE(T,NAME) \
  typedef union NAME { \
    DPAUCS_ringbuffer_base_t super; \
    struct { \
      DPAUCS_TYPED_RINGBUFFER_MEMBER_TEMPLATE(T) \
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

#define DPAUCS_RINGBUFFER_GET(buf) (buf)->buffer[ DPAUCS_ringbuffer_increment_read(&(buf)->super) ]
#define DPAUCS_RINGBUFFER_PUT(buf,x) ( ( DPAUCS_ringbuffer_full(&(buf)->super) ) \
    ? false : ( ( (buf)->buffer[ DPAUCS_ringbuffer_increment_write(&(buf)->super) ] = (x) ), true ) \
  )

#define DPAUCS_ringbuffer_eof(buf) ( !(buf)->range.size )
#define DPAUCS_ringbuffer_size(buf) ( (buf)->range.size )
#define DPAUCS_ringbuffer_full(buf) ( (buf)->range.size == (buf)->size )
#define DPAUCS_ringbuffer_begin(buf) (&(buf)->buffer[ (buf)->range.offset - (buf)->inverse ])
#define DPAUCS_ringbuffer_remaining(buf) ( (buf)->size - (buf)->range.size )

typedef struct DPAUCS_buffer_range {
  size_t offset;
  size_t size;
} DPAUCS_buffer_range_t;

typedef struct DPAUCS_ringbuffer_base {
  DPAUCS_TYPED_RINGBUFFER_MEMBER_TEMPLATE(void)
} DPAUCS_ringbuffer_base_t;

void DPAUCS_ringbuffer_reset( DPAUCS_ringbuffer_base_t* ringbuffer );
size_t DPAUCS_ringbuffer_copy( DPAUCS_ringbuffer_base_t* destination, DPAUCS_ringbuffer_base_t* source, size_t size );
size_t DPAUCS_ringbuffer_read( DPAUCS_ringbuffer_base_t* ringbuffer, void* destination, size_t size );
size_t DPAUCS_ringbuffer_write( DPAUCS_ringbuffer_base_t* ringbuffer, const void* source, size_t size );
void DPAUCS_ringbuffer_reverse( DPAUCS_ringbuffer_base_t* ringbuffer );
size_t DPAUCS_ringbuffer_skip_read( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPAUCS_ringbuffer_skip_write( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPAUCS_ringbuffer_rewind_read( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPAUCS_ringbuffer_rewind_write( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size );
size_t DPAUCS_ringbuffer_increment_read( DPAUCS_ringbuffer_base_t* ringbuffer ); // post increment
size_t DPAUCS_ringbuffer_increment_write( DPAUCS_ringbuffer_base_t* ringbuffer ); // pre increment
size_t DPAUCS_ringbuffer_decrement_read( DPAUCS_ringbuffer_base_t* ringbuffer ); // pre decrement
size_t DPAUCS_ringbuffer_decrement_write( DPAUCS_ringbuffer_base_t* ringbuffer ); // post decrement

DPAUCS_RINGBUFFER_TEMPLATE( char, DPAUCS_char_ringbuffer );
DPAUCS_RINGBUFFER_TEMPLATE( unsigned char, DPAUCS_uchar_ringbuffer );
DPAUCS_RINGBUFFER_TEMPLATE( struct DPAUCS_bufferInfo, DPAUCS_buffer_ringbuffer );

