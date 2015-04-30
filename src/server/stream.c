#include <stream.h>

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

static size_t stream_read_from_array( bufferInfo_t* info, void* p, size_t max_size ){
  (void)info;
  (void)p;
  (void)max_size;
  return 0;
}

size_t DPAUCS_stream_read( stream_t*const stream, void* p, size_t max_size ){
  while( max_size && !BUFFER_EOF(stream->buffer_buffer) ){
    bufferInfo_t* info = &BUFFER_GET(stream->buffer_buffer);
    switch( info->type ){
      case BUFFER_ARRAY:
        max_size -= stream_read_from_array(info,p,max_size);
      break;
      default: break;
    }
  }
  return true;
}
