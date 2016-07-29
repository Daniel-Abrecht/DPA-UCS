#ifndef DPAUCS_STREAM_H
#define DPAUCS_STREAM_H

#include <stdbool.h>
#include <DPA/UCS/ringbuffer.h>

#define DPAUCS_BUFFER_BUFFER_USERDEFINED(buf) ( DPAUCS_BUFFER_GET(buf).type >= BUFFER_TYPE_SIZE )
#define DPAUCS_BUFFER_BUFFER_TYPE(buf) ( DPAUCS_BUFFER_GET(buf).type - BUFFER_TYPE_SIZE )
#define DPAUCS_BUFFER_BUFFER_SIZE(buf) ( DPAUCS_BUFFER_GET(buf).size )
#define DPAUCS_BUFFER_BUFFER_PTR(buf) ( DPAUCS_BUFFER_GET(buf).ptr )

enum DPAUCS_buffer_type {
  BUFFER_BUFFER,
  BUFFER_ARRAY,
  BUFFER_TYPE_SIZE
};

typedef struct DPAUCS_bufferInfo {
  enum DPAUCS_buffer_type type;
  DPAUCS_buffer_range_t range;
  void* ptr;
} DPAUCS_bufferInfo_t;

typedef struct DPAUCS_stream {
  DPAUCS_uchar_ringbuffer_t* buffer;
  DPAUCS_buffer_ringbuffer_t* buffer_buffer;
} DPAUCS_stream_t;

typedef struct DPAUCS_stream_offsetStorage {
  size_t bufferOffset;
  size_t bufferBufferSize;
  size_t bufferBufferInfoOffset;
} DPAUCS_stream_offsetStorage_t;

typedef DPAUCS_bufferInfo_t DPAUCS_streamEntry_t;

typedef struct DPAUCS_stream_raw {
  size_t bufferBufferSize, charBufferSize;
  DPAUCS_bufferInfo_t* bufferBuffer;
  unsigned char* charBuffer;
} DPAUCS_stream_raw_t;

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t*, const void*, size_t );
bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t*, const void*, size_t );
void DPAUCS_stream_reset( DPAUCS_stream_t* );
size_t DPAUCS_stream_read( DPAUCS_stream_t*, void*, size_t );
void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros );
void DPAUCS_stream_saveWriteOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreWriteOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros );
size_t DPAUCS_stream_getLength( const DPAUCS_stream_t* stream, size_t max_ret, bool* has_more ); // inefficient
size_t DPAUCS_stream_raw_getLength( const DPAUCS_stream_raw_t* stream, size_t max_ret, bool* has_more ); // inefficient
void DPAUCS_stream_seek( DPAUCS_stream_t* stream, size_t size );

/* Be careful with the following functions */
bool DPAUCS_stream_to_raw_buffer( const DPAUCS_stream_t* stream, DPAUCS_stream_raw_t* raw );
void DPAUCS_raw_stream_truncate( DPAUCS_stream_raw_t* raw, size_t size );
void DPAUCS_raw_as_stream( DPAUCS_stream_raw_t* raw, void(*func)( DPAUCS_stream_t* stream, void* ptr ), void* ptr );
void DPAUCS_stream_swapEntries( DPAUCS_streamEntry_t* a, DPAUCS_streamEntry_t* b  );
DPAUCS_streamEntry_t* DPAUCS_stream_getEntry( DPAUCS_stream_t* stream );
bool DPAUCS_stream_nextEntry( DPAUCS_stream_t* stream );
bool DPAUCS_stream_previousEntry( DPAUCS_stream_t* stream );
/*-----------------------------------------*/


#define DPAUCS_stream_eof(s) DPAUCS_ringbuffer_eof(&(s)->buffer_buffer->super)

#endif
