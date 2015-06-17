#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <buffer.h>

typedef struct {
  uchar_buffer_t* buffer;
  buffer_buffer_t* buffer_buffer;
} DPAUCS_stream_t;

typedef struct {
  size_t bufferOffset;
  size_t bufferBufferOffset;
} DPAUCS_stream_offsetStorage_t;

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t*const, void*, size_t );
bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t*const, void*, size_t );
void DPAUCS_stream_reset( DPAUCS_stream_t*const );
size_t DPAUCS_stream_read( DPAUCS_stream_t*const, void*, size_t );
void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, DPAUCS_stream_offsetStorage_t* sros );
void DPAUCS_stream_saveWriteOffset( DPAUCS_stream_offsetStorage_t* sros, DPAUCS_stream_t* stream );
void DPAUCS_stream_restoreWriteOffset( DPAUCS_stream_t* stream, DPAUCS_stream_offsetStorage_t* sros );
size_t DPAUCS_stream_getLength( DPAUCS_stream_t* stream );

#define DPAUCS_stream_eof(s) BUFFER_EOF((s)->buffer_buffer)

#endif