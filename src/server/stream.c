#include <string.h>
#include <stdint.h>
#include <DPA/UCS/stream.h>
#include <DPA/UCS/logger.h>

void DPAUCS_stream_reset( DPAUCS_stream_t* stream ){
  DPAUCS_ringbuffer_reset( &stream->buffer_buffer->super );
  DPAUCS_ringbuffer_reset( &stream->buffer->super );
}

void DPAUCS_stream_saveReadOffset( DPAUCS_stream_offsetStorage_t* sros, const DPAUCS_stream_t* stream ){
  DPAUCS_bufferInfo_t* binf = DPAUCS_ringbuffer_begin(stream->buffer_buffer);
  sros->bufferBufferSize = stream->buffer_buffer->range.size;
  sros->bufferBufferInfoOffset = binf->range.offset;
}

void DPAUCS_stream_restoreReadOffset( DPAUCS_stream_t* stream, const DPAUCS_stream_offsetStorage_t* sros ){
  size_t obbs = stream->buffer_buffer->range.size;
  size_t nbbs = sros->bufferBufferSize;
  if( obbs > nbbs ){
//    rewindReadBuffer( stream->buffer_buffer, obbs - nbbs );
  }else{
//    DPAUCS_ringbuffer_rewind_read( &stream->buffer_buffer->super, nbbs - obbs );
  }
//  DPAUCS_BUFFER_BEGIN( stream->buffer_buffer )->offset = sros->bufferBufferInfoOffset;
}

void DPAUCS_stream_swapEntries( DPAUCS_streamEntry_t* a, DPAUCS_streamEntry_t* b  ){
  if( a == b )
    return;
  DPAUCS_streamEntry_t c = *a;
  *a = *b;
  *b = c;
}

DPAUCS_streamEntry_t* DPAUCS_stream_getEntry( DPAUCS_stream_t* stream ){
  return DPAUCS_ringbuffer_begin( stream->buffer_buffer );
}

bool DPAUCS_stream_nextEntry( DPAUCS_stream_t* stream ){
  if(DPAUCS_ringbuffer_eof( &stream->buffer_buffer->super ))
    return false;
  DPAUCS_ringbuffer_increment_read( &stream->buffer_buffer->super );
  return true;
}

bool DPAUCS_stream_previousEntry( DPAUCS_stream_t* stream ){
  if(DPAUCS_ringbuffer_full( &stream->buffer_buffer->super ))
    return false;
  DPAUCS_ringbuffer_decrement_read( &stream->buffer_buffer->super );
  return true;
}

bool DPAUCS_stream_to_raw_buffer( const DPAUCS_stream_t* stream, DPAUCS_stream_raw_t* raw ){

  DPAUCS_uchar_ringbuffer_t cb = *stream->buffer;
  DPAUCS_buffer_ringbuffer_t bb = *stream->buffer_buffer;

  size_t cb_size = DPAUCS_ringbuffer_size(&cb.super);
  size_t bb_size = DPAUCS_ringbuffer_size(&bb.super);
  if( cb_size > raw->charBufferSize || bb_size > raw->bufferBufferSize )
    return false;

  DPAUCS_ringbuffer_read( &cb.super, raw->charBuffer + raw->charBufferSize - cb_size, cb_size );
  DPAUCS_ringbuffer_read( &bb.super, raw->bufferBuffer, bb_size );

  return true;
}

void DPAUCS_raw_stream_truncate( DPAUCS_stream_raw_t* raw, size_t size ){
  size_t cbskip = 0;
  DPAUCS_bufferInfo_t *bb, *end;
  for( bb=raw->bufferBuffer, end=bb+raw->bufferBufferSize; bb<end; bb++ ){
    if( !size ) break;
    if( bb->range.size > size ){
      switch( bb->type ){
        case BUFFER_BUFFER: cbskip  += size; break;
        case BUFFER_ARRAY : *(char**)&bb->ptr += size; break;
        default: break;
      }
      bb->range.size -= size;
    }else{
      if( bb->type == BUFFER_BUFFER )
        cbskip += bb->range.size;
      size -= bb->range.size;
    }
  }
  raw->bufferBufferSize -= bb - raw->bufferBuffer;
  raw->bufferBuffer = bb;
  if( raw->charBufferSize >= cbskip ){
    raw->charBufferSize -= cbskip;
  }else{
    raw->charBufferSize = 0;
  }
}

void DPAUCS_raw_as_stream( DPAUCS_stream_raw_t* raw, void(*func)(DPAUCS_stream_t*,void*), void* ptr ){

  DPAUCS_uchar_ringbuffer_t buffer = {
    .super = {
      .size = raw->charBufferSize,
      .range = {
        .offset = raw->charBufferSize,
        .size = raw->charBufferSize
      },
      .inverse = true,
      .type_size = sizeof( *buffer.buffer ),
      .buffer = raw->charBuffer,
    }
  };

  DPAUCS_buffer_ringbuffer_t buffer_buffer = {
    .super = {
      .size = raw->bufferBufferSize,
      .range = {
        .offset = 0,
        .size = raw->bufferBufferSize
      },
      .inverse = false,
      .buffer = raw->bufferBuffer,
      .type_size = sizeof( *buffer_buffer.buffer )
    }
  };

  DPAUCS_stream_t stream = {
    .buffer = &buffer,
    .buffer_buffer = &buffer_buffer
  };

  (*func)( &stream, ptr );

}

bool DPAUCS_stream_copyWrite( DPAUCS_stream_t* stream, const void* p, size_t size ){
  if( DPAUCS_ringbuffer_full(&stream->buffer_buffer->super)
   || DPAUCS_ringbuffer_remaining(&stream->buffer->super) < size
  ) return false;
  DPAUCS_bufferInfo_t entry = {0};
  entry.type = BUFFER_BUFFER;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.ptr = 0;
  DPAUCS_RINGBUFFER_PUT(stream->buffer_buffer,entry);
  DPAUCS_ringbuffer_write(&stream->buffer->super,p,size);
  return true;
}

bool DPAUCS_stream_referenceWrite( DPAUCS_stream_t* stream, const void* p, size_t size ){
  if(DPAUCS_ringbuffer_full(&stream->buffer_buffer->super))
    return false;
  DPAUCS_bufferInfo_t entry = {0};
  entry.type = BUFFER_ARRAY;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.ptr = (void*)p;
  DPAUCS_RINGBUFFER_PUT( stream->buffer_buffer, entry );
  return true;
}

static inline size_t stream_read_from_buffer( DPAUCS_bufferInfo_t* info, void* p, size_t max_size ){
  size_t size = info->range.size - info->range.offset;
  size_t amount = size < max_size ? size : max_size;
  size_t read = DPAUCS_ringbuffer_read(info->ptr,p,amount);
  info->range.offset += read;
  if( read != amount ){
    info->range.size = read + info->range.offset;
    DPAUCS_LOG(
      "BUG: Ringbuffer doesn't contain es much data as expected."
      "This may happen if the streambuffers are modified without using the stream API or"
      "By using the stream API in unintended ways."
    );
  }
  return read;
}

static inline size_t stream_read_from_array( DPAUCS_bufferInfo_t* info, void* p, size_t max_size ){
  size_t size = info->range.size - info->range.offset;
  size_t n = size < max_size ? size : max_size;
  memcpy(p,(char*)info->ptr+info->range.offset,n);
  info->range.offset += n;
  return n;
}

size_t DPAUCS_stream_read( DPAUCS_stream_t* stream, void* p, size_t max_size ){
  size_t n = max_size;
  while( n && !DPAUCS_ringbuffer_eof(&stream->buffer_buffer->super) ){
    DPAUCS_bufferInfo_t* info = DPAUCS_ringbuffer_begin(stream->buffer_buffer);
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
    if( info->range.offset >= info->range.size )
      DPAUCS_ringbuffer_increment_read(&stream->buffer_buffer->super);
  }
  return max_size - n;
}

void DPAUCS_stream_seek( DPAUCS_stream_t* stream, size_t size ){
  while( size && !DPAUCS_ringbuffer_eof(&stream->buffer_buffer->super) ){
    DPAUCS_bufferInfo_t* info = DPAUCS_ringbuffer_begin(stream->buffer_buffer);
    if( info->range.size - info->range.offset <= size ){
      size -= info->range.size - info->range.offset;
      DPAUCS_ringbuffer_increment_read( &stream->buffer_buffer->super );
    }else{
      info->range.offset += size;
      break;
    }
  }
}

/*********************************************************************************************
* A stream can contain more than SIZE_MAX bytes of datas which would lead to an overflow if  *
* not preventet. I solved this problem by Specifying an upper limit for the return value and *
* an returning an information if there would have been more datas.                           *
* This function is slow, avoid if possible                                                   *
**********************************************************************************************/
size_t DPAUCS_stream_getLength( const DPAUCS_stream_t* stream, size_t max_ret, bool* has_more ){
  DPAUCS_buffer_ringbuffer_t tmp = *stream->buffer_buffer;
  size_t n = 0;
  while( !DPAUCS_ringbuffer_eof(&tmp.super) ){
    size_t s = DPAUCS_RINGBUFFER_GET( stream->buffer_buffer ).range.size;
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
    size_t s = stream->bufferBuffer[i].range.size;
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
