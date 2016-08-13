#ifndef DPA_STREAM_H
#define DPA_STREAM_H

#include <stdbool.h>
#include <DPA/UCS/ringbuffer.h>

#define DPA_BUFFER_BUFFER_USERDEFINED(buf) ( DPA_BUFFER_GET(buf).type >= BUFFER_TYPE_SIZE )
#define DPA_BUFFER_BUFFER_TYPE(buf) ( DPA_BUFFER_GET(buf).type - BUFFER_TYPE_SIZE )
#define DPA_BUFFER_BUFFER_SIZE(buf) ( DPA_BUFFER_GET(buf).size )
#define DPA_BUFFER_BUFFER_PTR(buf) ( DPA_BUFFER_GET(buf).ptr )

enum DPA_buffer_type {
  BUFFER_BUFFER,
  BUFFER_ARRAY,
  BUFFER_TYPE_SIZE
};

typedef struct DPA_bufferInfo {
  enum DPA_buffer_type type;
  DPA_buffer_range_t range;
  void* ptr;
} DPA_bufferInfo_t;

typedef struct DPA_stream {
  DPA_uchar_ringbuffer_t* buffer;
  DPA_buffer_ringbuffer_t* buffer_buffer;
} DPA_stream_t;

typedef struct DPA_stream_offsetStorage {
  size_t bufferOffset;
  size_t bufferBufferSize;
  size_t bufferBufferInfoOffset;
} DPA_stream_offsetStorage_t;

typedef DPA_bufferInfo_t DPA_streamEntry_t;

typedef struct DPA_stream_raw {
  size_t bufferBufferSize, charBufferSize;
  DPA_bufferInfo_t* bufferBuffer;
  unsigned char* charBuffer;
} DPA_stream_raw_t;

bool DPA_stream_copyWrite( DPA_stream_t*, const void*, size_t );
bool DPA_stream_referenceWrite( DPA_stream_t*, const void*, size_t );
void DPA_stream_reset( DPA_stream_t* );
size_t DPA_stream_read( DPA_stream_t*, void*, size_t );
void DPA_stream_saveReadOffset( DPA_stream_offsetStorage_t* sros, const DPA_stream_t* stream );
void DPA_stream_restoreReadOffset( DPA_stream_t* stream, const DPA_stream_offsetStorage_t* sros );
void DPA_stream_saveWriteOffset( DPA_stream_offsetStorage_t* sros, const DPA_stream_t* stream );
void DPA_stream_restoreWriteOffset( DPA_stream_t* stream, const DPA_stream_offsetStorage_t* sros );
size_t DPA_stream_getLength( const DPA_stream_t* stream, size_t max_ret, bool* has_more ); // inefficient
size_t DPA_stream_raw_getLength( const DPA_stream_raw_t* stream, size_t max_ret, bool* has_more ); // inefficient
void DPA_stream_seek( DPA_stream_t* stream, size_t size );

/* Be careful with the following functions */
bool DPA_stream_to_raw_buffer( const DPA_stream_t* stream, DPA_stream_raw_t* raw );
void DPAUCS_raw_stream_truncate( DPA_stream_raw_t* raw, size_t size );
void DPAUCS_raw_as_stream( DPA_stream_raw_t* raw, void(*func)( DPA_stream_t* stream, void* ptr ), void* ptr );
void DPA_stream_swapEntries( DPA_streamEntry_t* a, DPA_streamEntry_t* b  );
DPA_streamEntry_t* DPA_stream_getEntry( DPA_stream_t* stream );
bool DPA_stream_nextEntry( DPA_stream_t* stream );
bool DPA_stream_previousEntry( DPA_stream_t* stream );
/*-----------------------------------------*/


#define DPA_stream_eof(s) DPA_ringbuffer_eof(&(s)->buffer_buffer->super)

#endif
