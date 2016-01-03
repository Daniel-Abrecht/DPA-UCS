#include <string.h>
#include <stdint.h>
#include <DPA/UCS/stream.h>

void DPAUCS_stream_reset( DPAUCS_stream_t* stream ){
  DPAUCS_stream_offsetStorage_t sros;
  memset( &sros, 0, sizeof(sros) );
  DPAUCS_stream_restoreWriteOffset( stream, &sros );
  DPAUCS_stream_restoreReadOffset ( stream, &sros );
}

void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream ){
  bufferInfo_t* binf = DPAUCS_BUFFER_BEGIN(stream->buffer_buffer);
  sros->bufferOffset = 0;
  switch( binf->type ){
    case BUFFER_BUFFER: {
      sros->bufferOffset = DPAUCS_BUFFER_GET_OFFSET( (uchar_buffer_t*)binf->ptr, true );
    } break;
    default: break;
  }
  sros->bufferBufferOffset = DPAUCS_BUFFER_GET_OFFSET( stream->buffer_buffer, true );
  sros->bufferBufferInfoOffset = binf->offset;
}

void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros ){
  DPAUCS_BUFFER_BEGIN(stream->buffer_buffer)->offset = 0;
  DPAUCS_BUFFER_SET_OFFSET( stream->buffer_buffer, true, sros->bufferBufferOffset );
  bufferInfo_t* binf = DPAUCS_BUFFER_BEGIN(stream->buffer_buffer);
  switch( binf->type ){
    case BUFFER_BUFFER: {
      DPAUCS_BUFFER_SET_OFFSET( (uchar_buffer_t*)binf->ptr, true, sros->bufferOffset );
    } break;
    default: break;
  }
  binf->offset = sros->bufferBufferInfoOffset;
}

void DPAUCS_stream_saveWriteOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream ){
  sros->bufferOffset = DPAUCS_BUFFER_GET_OFFSET( stream->buffer, false );
  sros->bufferBufferOffset = DPAUCS_BUFFER_GET_OFFSET( stream->buffer_buffer, false );
  sros->bufferBufferInfoOffset = 0;
}

void DPAUCS_stream_restoreWriteOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros ){
  DPAUCS_BUFFER_SET_OFFSET( stream->buffer, false, sros->bufferOffset );
  DPAUCS_BUFFER_SET_OFFSET( stream->buffer_buffer, false, sros->bufferBufferOffset );
}

void DPAUCS_stream_swapEntries( streamEntry_t* a, streamEntry_t* b  ){
  if( a == b )
    return;
  streamEntry_t c = *a;
  *a = *b;
  *b = c;
}

streamEntry_t* DPAUCS_stream_getEntry( DPAUCS_stream_t* stream ){
  return DPAUCS_BUFFER_BEGIN( stream->buffer_buffer );
}

bool DPAUCS_stream_skipEntry( DPAUCS_stream_t* stream ){
  if(DPAUCS_BUFFER_EOF( stream->buffer_buffer ))
    return false;
  DPAUCS_BUFFER_GET(stream->buffer_buffer).offset = 0;
  return true;
}

bool DPAUCS_stream_reverseSkipEntry( DPAUCS_stream_t* stream ){
  DPAUCS_BUFFER_BEGIN( stream->buffer_buffer )->offset = 0;
  DPAUCS_BUFFER_SKIP( stream->buffer_buffer, 1, true );
  return true;
}

bool DPAUCS_stream_to_raw_buffer( DPAUCS_stream_t* stream, DPAUCS_stream_raw_t* raw ){

  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveReadOffset( &sros, stream );

  uchar_buffer_t* cb = stream->buffer;
  buffer_buffer_t* bb = stream->buffer_buffer;

  if( DPAUCS_BUFFER_SIZE(cb) > raw->charBufferSize || DPAUCS_BUFFER_SIZE(bb) > raw->bufferBufferSize )
    return false;

  { // Raw char buffer in reverse order, allows to avoid some memory movements elsewere
    unsigned char* c = raw->charBuffer + raw->charBufferSize;
    while(!DPAUCS_BUFFER_EOF( cb ))
      *--c = DPAUCS_BUFFER_GET( cb );
  }

  {
    bufferInfo_t* b = raw->bufferBuffer;
    while(!DPAUCS_BUFFER_EOF( bb )){
      *b = DPAUCS_BUFFER_GET( bb );
      switch( b->type ){
        case BUFFER_BUFFER: {
          if( b->ptr == cb )
            b->ptr = 0;
        } break;
        default: break;
      }
      b++;
    }
  }

  DPAUCS_stream_restoreReadOffset( stream, &sros );

  return true;
}

void DPAUCS_stream_prepare_from_buffer( DPAUCS_stream_raw_t* raw, size_t count, void(*func)( DPAUCS_stream_t* stream ) ){

  (void)raw;
  (void)count;
  (void)func;

}

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t* stream, const void* p, size_t size ){
  if( DPAUCS_BUFFER_FULL(stream->buffer_buffer) )
    return false;
  if( stream->buffer->state.size - DPAUCS_BUFFER_SIZE(stream->buffer) < size )
    return false;
  const unsigned char* buff = p;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_BUFFER;
  entry.size = size;
  entry.offset = 0;
  entry.ptr = stream->buffer;
  DPAUCS_BUFFER_PUT(stream->buffer_buffer,entry);
  while( size-- )
    DPAUCS_BUFFER_PUT(stream->buffer,*(buff++));
  return true;
}

bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t* stream, const void* p, size_t size ){
  if(DPAUCS_BUFFER_FULL(stream->buffer_buffer))
    return false;
  bufferInfo_t entry = {0};
  entry.type = BUFFER_ARRAY;
  entry.size = size;
  entry.offset = 0;
  entry.ptr = p;
  DPAUCS_BUFFER_PUT(stream->buffer_buffer,entry);
  return true;
}

static inline size_t stream_read_from_buffer( bufferInfo_t* info, void* p, size_t max_size ){
  unsigned char* uch = p;
  size_t size = info->size - info->offset;
  uchar_buffer_t* buffer = (uchar_buffer_t*)info->ptr;
  size_t i, n;
  for(
    i = 0, n = size < max_size ? size : max_size;
    i < n;
    i++
  ) *(uch++) = DPAUCS_BUFFER_AT(buffer,i+info->offset);
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

size_t DPAUCS_stream_read( DPAUCS_stream_t* stream, void* p, size_t max_size ){
  size_t n = max_size;
  while( n && !DPAUCS_BUFFER_EOF(stream->buffer_buffer) ){
    bufferInfo_t* info = DPAUCS_BUFFER_BEGIN(stream->buffer_buffer);
    size_t size = 0;
    switch( info->type ){
      case BUFFER_ARRAY:
        size = stream_read_from_array(info,p,n);
      break;
      case BUFFER_BUFFER:
        size = stream_read_from_buffer(info,p,n);
      break;
      default: return max_size - n;
    }
    n -= size;
    p = (char*)p+size;
    if( info->offset >= info->size ){
      info->offset = 0;
      DPAUCS_BUFFER_INCREMENT(stream->buffer_buffer);
    }
  }
  return max_size - n;
}

void DPAUCS_stream_seek( DPAUCS_stream_t* stream, size_t size ){
  while( !DPAUCS_BUFFER_EOF(stream->buffer_buffer) ){
    bufferInfo_t* info = DPAUCS_BUFFER_BEGIN(stream->buffer_buffer);
    if( info->size <= size ){
      info->offset = 0;
      size -= info->size;
      DPAUCS_BUFFER_INCREMENT( stream->buffer_buffer );
    }else{
      info->offset = size;
      break;
    }
  }
}

/*********************************************************************************************
* A stream can contain more than SIZE_MAX bytes of datas which would leave to an overflow if *
* not preventet. I solved this problem by Specifying an upper limit for the return value and *
* an returning an information if there would have been more datas.                           *
**********************************************************************************************/
size_t DPAUCS_stream_getLength( const DPAUCS_stream_t* stream, size_t max_ret, bool* has_more ){
  size_t i = DPAUCS_BUFFER_SIZE( stream->buffer_buffer );
  size_t n = 0;
  while( i-- ){
    size_t s = DPAUCS_BUFFER_AT( stream->buffer_buffer, i ).size;
    if( s + n < n || s + n > max_ret ){
      if(has_more)
        *has_more = true;
      return max_ret;
    }
    n += s;
  }
  *has_more = false;
  return n;
}

size_t DPAUCS_stream_raw_getLength( const DPAUCS_stream_raw_t* stream, size_t max_ret, bool* has_more ){
  size_t i = stream->bufferBufferSize;
  size_t n = 0;
  while( i-- ){
    size_t s = stream->bufferBuffer[i].size;
    if( s + n < n || s + n > max_ret ){
      if(has_more)
        *has_more = true;
      return max_ret;
    }
    n += s;
  }
  *has_more = false;
  return n;
}
