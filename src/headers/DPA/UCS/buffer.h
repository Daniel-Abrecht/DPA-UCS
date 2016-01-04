#include <stddef.h>
#include <stdbool.h>

#define DPAUCS_BUFFER_TEMPLATE(T) \
  typedef struct { \
    DPAUCS_ringbuffer_state_t state; \
    T*const buffer; \
  }

#define DPAUCS_DEFINE_BUFFER(B,T,name,S,I) \
  static B name ## _buffer[S+1]; \
  T name = { \
    .state = { \
      .size = S, \
      .offset = { \
        .start = I ? S - 1 : 0, \
        .end   = I ? S - 1 : 0 \
      }, \
      .empty = true, \
      .inverse = I \
    }, \
    .buffer = name ## _buffer \
  }

#define DPAUCS_BUFFER_BEGIN(buf) \
  ( &(buf)->buffer[ (buf)->state.inverse ? (buf)->state.offset.end : (buf)->state.offset.start ] )
#define DPAUCS_BUFFER_SIZE(buf) DPAUCS_buffer_size(&(buf)->state)
#define DPAUCS_BUFFER_EOF(buf) ( (buf)->state.empty && (buf)->state.offset.start == (buf)->state.offset.end )
#define DPAUCS_BUFFER_SKIP(buf,len,reverse) DPAUCS_buffer_skip( &(buf)->state, len, reverse )
#define DPAUCS_BUFFER_REVERSE(buf) do { (buf)->state.inverse = !(buf)->state.inverse; } while(0)
#define DPAUCS_BUFFER_GET(buf) (buf)->buffer[ DPAUCS_buffer_increment(&(buf)->state,true) ]
#define DPAUCS_BUFFER_AT(buf,off) (buf)->buffer[ \
    ( ( (buf)->state.inverse ? (buf)->state.offset.end : (buf)->state.offset.start ) + (off) ) % (buf)->state.size \
  ]
#define DPAUCS_BUFFER_FULL(buf) ( !(buf)->state.empty && (buf)->state.offset.start == (buf)->state.offset.end )
#define DPAUCS_BUFFER_PUT(buf,x) ( ( DPAUCS_BUFFER_FULL(buf) ) \
    ? false : ( ( (buf)->buffer[ DPAUCS_buffer_increment(&(buf)->state,false) ] = (x) ), true ) \
  )
#define DPAUCS_BUFFER_GET_OFFSET(buf,readOrWrite) DPAUCS_buffer_get_offset( &(buf)->state, (readOrWrite) )
#define DPAUCS_BUFFER_SET_OFFSET(buf,readOrWrite,offset) DPAUCS_buffer_set_offset( &(buf)->state, (readOrWrite), (offset) )
#define DPAUCS_BUFFER_SIZE(buf) DPAUCS_buffer_size(&(buf)->state)
#define DPAUCS_BUFFER_INCREMENT(buf) DPAUCS_buffer_increment( &(buf)->state, true )

#define DPAUCS_BUFFER_MAX_SIZE ((size_t)~1)

#define DPAUCS_BUFFER_BUFFER_USERDEFINED(buf) ( DPAUCS_BUFFER_GET(buf).type >= BUFFER_TYPE_SIZE )
#define DPAUCS_BUFFER_BUFFER_TYPE(buf) ( DPAUCS_BUFFER_GET(buf).type - BUFFER_TYPE_SIZE )
#define DPAUCS_BUFFER_BUFFER_SIZE(buf) ( DPAUCS_BUFFER_GET(buf).size )
#define DPAUCS_BUFFER_BUFFER_PTR(buf) ( DPAUCS_BUFFER_GET(buf).ptr )

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

typedef struct DPAUCS_ringbuffer_state {
  const size_t size;
  struct {
    size_t start;
    size_t end;
  } offset;
  bool empty;
  bool inverse;
} DPAUCS_ringbuffer_state_t;

size_t DPAUCS_buffer_get_offset( DPAUCS_ringbuffer_state_t* state, bool readOrWrite );
size_t DPAUCS_buffer_increment( DPAUCS_ringbuffer_state_t* state, bool readOrWrite );
size_t DPAUCS_buffer_size( const DPAUCS_ringbuffer_state_t* state );
void DPAUCS_buffer_set_offset( DPAUCS_ringbuffer_state_t* state, bool readOrWrite, size_t offset );
void DPAUCS_buffer_skip( DPAUCS_ringbuffer_state_t* state, size_t s, bool reverse );

DPAUCS_BUFFER_TEMPLATE(char) char_buffer_t;
DPAUCS_BUFFER_TEMPLATE(unsigned char) uchar_buffer_t;
DPAUCS_BUFFER_TEMPLATE(bufferInfo_t) buffer_buffer_t;

