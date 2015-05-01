#include <stream.h>
#include <string.h>

void DPAUCS_stream_reset(stream_t*const stream){
  stream->buffer->write_offset = stream->outputStreamBufferWriteStartOffset;
  stream->buffer_buffer->write_offset = stream->outputStreamBufferBufferWriteStartOffset;
}

bool DPAUCS_stream_copyWrite( stream_t*const stream, void* p, size_t size ){
  if( BUFFER_FULL(stream->buffer_buffer) )
    return false;
  if( BUFFER_SIZE(stream->buffer) < size )
    return false;
  unsigned char* buff = p;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_BUFFER;
  entry.size = size;
  entry.ptr = stream->buffer;
  BUFFER_PUT(stream->buffer_buffer,entry);
  while( size-- )
    BUFFER_PUT(stream->buffer,*(buff++));
  return true;
}

bool DPAUCS_stream_referenceWrite( stream_t*const stream, void* p, size_t size ){
  if(BUFFER_FULL(stream->buffer_buffer))
    return false;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_ARRAY;
  entry.size = size;
  entry.ptr = p;
  BUFFER_PUT(stream->buffer_buffer,entry);
  return true;
}

static inline size_t stream_read_from_buffer( bufferInfo_t* info, void* p, size_t max_size ){
  unsigned char* uch = p;
  uchar_buffer_t* buffer = info->ptr;
  size_t i, n;
  for(
    i=0, n= info->size < max_size ? info->size : max_size;
    i < n && !BUFFER_EOF(buffer);
    i++
  ) *(uch++) = BUFFER_GET(buffer);
  return i;
}

static inline size_t stream_read_from_array( bufferInfo_t* info, void* p, size_t max_size ){
  size_t n = info->size < max_size ? info->size : max_size;
  memcpy(p,info->ptr,n);
  return n;
}

size_t DPAUCS_stream_read( stream_t*const stream, void* p, size_t max_size ){
  size_t n = max_size;
  while( n && !BUFFER_EOF(stream->buffer_buffer) ){
    bufferInfo_t* info = BUFFER_BEGIN(stream->buffer_buffer);
    switch( info->type ){
      case BUFFER_ARRAY:
        n -= stream_read_from_array(info,p,n);
      break;
      case BUFFER_BUFFER:
        n -= stream_read_from_array(info,p,n);
      break;
      default: return max_size - n;
    }
    BUFFER_SKIP(stream->buffer_buffer,1);
  }
  return max_size - n;
}
