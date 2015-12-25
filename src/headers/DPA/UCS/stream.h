#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <DPA/UCS/buffer.h>

typedef struct {
  uchar_buffer_t* buffer;
  buffer_buffer_t* buffer_buffer;
} DPAUCS_stream_t;

typedef struct {
  size_t bufferOffset;
  size_t bufferBufferOffset;
  size_t bufferBufferInfoOffset;
} DPAUCS_stream_offsetStorage_t;

typedef bufferInfo_t streamEntry_t;

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t*, const void*, size_t );
bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t*, const void*, size_t );
void DPAUCS_stream_reset( DPAUCS_stream_t* );
size_t DPAUCS_stream_read( DPAUCS_stream_t*, void*, size_t );
void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros );
void DPAUCS_stream_saveWriteOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreWriteOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros );
size_t DPAUCS_stream_getLength( const DPAUCS_stream_t* stream );
bool DPAUCS_stream_to_raw_buffer( DPAUCS_stream_t* stream, bufferInfo_t* bmem, size_t bmsize, unsigned char* cmem, size_t cmsize );
void DPAUCS_stream_seek( DPAUCS_stream_t* stream, size_t size );

/* Be careful with the following functions */
void DPAUCS_stream_swapEntries( streamEntry_t* a, streamEntry_t* b  );
streamEntry_t* DPAUCS_stream_getEntry( DPAUCS_stream_t* stream );
bool DPAUCS_stream_skipEntry( DPAUCS_stream_t* stream );
bool DPAUCS_stream_reverseSkipEntry( DPAUCS_stream_t* stream );
/*-----------------------------------------*/


#define DPAUCS_stream_eof(s) BUFFER_EOF((s)->buffer_buffer)

#endif