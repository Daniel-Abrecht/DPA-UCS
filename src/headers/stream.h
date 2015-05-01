#include <stdbool.h>
#include <buffer.h>

typedef struct {
  uchar_buffer_t* buffer;
  buffer_buffer_t* buffer_buffer;
  size_t outputStreamBufferWriteStartOffset;
  size_t outputStreamBufferBufferWriteStartOffset;
} stream_t;

bool DPAUCS_stream_copyWrite( stream_t*const, void*, size_t );
bool DPAUCS_stream_referenceWrite( stream_t*const, void*, size_t );
void DPAUCS_stream_reset( stream_t*const );
size_t DPAUCS_stream_read( stream_t*const, void*, size_t );

#define DPAUCS_stream_eof(s) BUFFER_EOF((s)->buffer_buffer)
