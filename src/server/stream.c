#include <stream.h>
#include <string.h>

void DPAUCS_stream_reset( DPAUCS_stream_t*const stream ){
  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveWriteOffset( &sros, stream );
  DPAUCS_stream_restoreReadOffset( stream, &sros );
  stream->buffer->write_offset = stream->buffer->read_offset;
  stream->buffer_buffer->write_offset = stream->buffer_buffer->read_offset;
}

void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, DPAUCS_stream_t* stream ){
  sros->bufferOffset = stream->buffer->read_offset;
  sros->bufferBufferOffset = stream->buffer_buffer->read_offset;
}

void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, DPAUCS_stream_offsetStorage_t* sros ){
  stream->buffer->read_offset = sros->bufferOffset;
  stream->buffer_buffer->read_offset = sros->bufferBufferOffset;
}

void DPAUCS_stream_saveWriteOffset( DPAUCS_stream_offsetStorage_t* sros, DPAUCS_stream_t* stream ){
  sros->bufferOffset = stream->buffer->write_offset;
  sros->bufferBufferOffset = stream->buffer_buffer->write_offset;
}

void DPAUCS_stream_restoreWriteOffset( DPAUCS_stream_t* stream, DPAUCS_stream_offsetStorage_t* sros ){
  stream->buffer->write_offset = sros->bufferOffset;
  stream->buffer_buffer->write_offset = sros->bufferBufferOffset;
}

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t*const stream, void* p, size_t size ){
  if( BUFFER_FULL(stream->buffer_buffer) )
    return false;
  if( BUFFER_SIZE(stream->buffer) < size )
    return false;
  unsigned char* buff = p;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_BUFFER;
  entry.size = size;
  entry.offset = 0;
  entry.ptr = stream->buffer;
  BUFFER_PUT(stream->buffer_buffer,entry);
  while( size-- )
    BUFFER_PUT(stream->buffer,*(buff++));
  return true;
}

bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t*const stream, void* p, size_t size ){
  if(BUFFER_FULL(stream->buffer_buffer))
    return false;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_ARRAY;
  entry.size = size;
  entry.offset = 0;
  entry.ptr = p;
  BUFFER_PUT(stream->buffer_buffer,entry);
  return true;
}

static inline size_t stream_read_from_buffer( bufferInfo_t* info, void* p, size_t max_size ){
  unsigned char* uch = p;
  size_t size = info->size - info->offset;
  uchar_buffer_t* buffer = info->ptr;
  size_t i, n;
  for(
    i = 0, n = size < max_size ? size : max_size;
    i < n;
    i++
  ) *(uch++) = BUFFER_AT(buffer,i+info->offset);
  info->offset += i;
  return i;
}

static inline size_t stream_read_from_array( bufferInfo_t* info, void* p, size_t max_size ){
  size_t size = info->size - info->offset;
  size_t n = size < max_size ? size : max_size;
  memcpy(p,(char*)info->ptr+info->offset,n);
  info->offset += n;
  return n;
}

size_t DPAUCS_stream_read( DPAUCS_stream_t*const stream, void* p, size_t max_size ){
  size_t n = max_size;
  while( n && !BUFFER_EOF(stream->buffer_buffer) ){
    bufferInfo_t* info = BUFFER_BEGIN(stream->buffer_buffer);
    switch( info->type ){
      case BUFFER_ARRAY:
        n -= stream_read_from_array(info,p,n);
      break;
      case BUFFER_BUFFER:
        n -= stream_read_from_buffer(info,p,n);
      break;
      default: return max_size - n;
    }
    if( info->offset >= info->size ){
      info->offset = 0;
      BUFFER_SKIP(stream->buffer_buffer,1);
    }
  }
  return max_size - n;
}

size_t DPAUCS_stream_getLength( DPAUCS_stream_t* stream ){
  size_t i = BUFFER_SIZE( stream->buffer_buffer );
  size_t n = 0;
  while( i-- )
    n += BUFFER_AT( stream->buffer_buffer, i ).size;
  return n;
}