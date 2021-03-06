#include <string.h>
#include <stdint.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/stream.h>
#include <DPA/utils/logger.h>

void DPA_stream_reset( DPA_stream_t* stream ){
  DPA_ringbuffer_reset( &stream->buffer_buffer->super );
  DPA_ringbuffer_reset( &stream->buffer->super );
}

void DPA_stream_restoreReadOffset( DPA_stream_t* stream, size_t old_size ){
  size_t new_size = DPA_stream_getLength( stream, ~0, 0 );
  if( old_size > new_size ){
    DPA_stream_rewind( stream, old_size - new_size );
  }else{
    DPA_stream_seek( stream, new_size - old_size );
  }
}

bool DPA_stream_to_raw_buffer( const DPA_stream_t* stream, DPA_stream_raw_t* raw ){

  DPA_uchar_ringbuffer_t cb = *stream->buffer;
  DPA_buffer_ringbuffer_t bb = *stream->buffer_buffer;

  size_t cb_size = DPA_ringbuffer_size(&cb.super);
  size_t bb_size = DPA_ringbuffer_size(&bb.super);
  if( cb_size > raw->charBufferSize || bb_size > raw->bufferBufferSize )
    return false;

  DPA_ringbuffer_reverse( &bb.super );
  DPA_ringbuffer_read( &cb.super, raw->charBuffer, cb_size );
  DPA_ringbuffer_read( &bb.super, raw->bufferBuffer, bb_size );

  return true;
}

void DPAUCS_raw_stream_truncate( DPA_stream_raw_t* raw, size_t size ){
  size_t cbskip = 0;
  DPA_bufferInfo_t *bb, *end;
  for( bb=raw->bufferBuffer, end=bb+raw->bufferBufferSize; bb<end; bb++ ){
    if( !size ) break;
    if( bb->range.size > size ){
      switch( bb->type ){
        case BUFFER_BUFFER: cbskip  += size; break;
        case BUFFER_ARRAY : *(const char**)&bb->cptr += size; break;
#ifdef __FLASH
        case BUFFER_FLASH : *(const flash char**)&bb->fptr += size; break;
#endif
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

void DPAUCS_raw_as_stream( DPA_stream_raw_t* raw, void(*func)(DPA_stream_t*,void*), void* ptr ){

  DPA_LOG("DPAUCS_raw_as_stream: cbs: %"PRIuSIZE", bbs: %"PRIuSIZE", cb: %p, bb: %p\n",raw->charBufferSize,raw->bufferBufferSize,(void*)raw->charBuffer,(void*)raw->bufferBuffer);

  DPA_uchar_ringbuffer_t buffer = {
    .super = {
      .size = raw->charBufferSize,
      .range = {
        .offset = 0,
        .size = raw->charBufferSize
      },
      .inverse = false,
      .type_size = sizeof( *buffer.buffer ),
      .buffer = raw->charBuffer,
    }
  };

  DPA_buffer_ringbuffer_t buffer_buffer = {
    .super = {
      .size = raw->bufferBufferSize,
      .range = {
        .offset = raw->bufferBufferSize,
        .size = raw->bufferBufferSize
      },
      .inverse = true,
      .buffer = raw->bufferBuffer,
      .type_size = sizeof( *buffer_buffer.buffer )
    }
  };

  for( size_t i=0; i<raw->bufferBufferSize; i++ )
    raw->bufferBuffer[i].range.offset = 0;

  DPA_stream_t stream = {
    .buffer = &buffer,
    .buffer_buffer = &buffer_buffer
  };

  (*func)( &stream, ptr );

}

bool DPA_stream_copyWrite( DPA_stream_t* stream, const void* p, size_t size ){
  if( DPA_ringbuffer_full(&stream->buffer_buffer->super)
   || DPA_ringbuffer_remaining(&stream->buffer->super) < size
  ) return false;
  DPA_bufferInfo_t entry = {0};
  entry.type = BUFFER_BUFFER;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.ptr = 0;
  DPA_RINGBUFFER_PUT(stream->buffer_buffer,entry);
  DPA_ringbuffer_write(&stream->buffer->super,p,size);
  return true;
}

bool DPA_stream_referenceWrite( DPA_stream_t* stream, const void* p, size_t size ){
  if(DPA_ringbuffer_full(&stream->buffer_buffer->super))
    return false;
  DPA_bufferInfo_t entry = {0};
  entry.type = BUFFER_ARRAY;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.cptr = p;
  DPA_RINGBUFFER_PUT( stream->buffer_buffer, entry );
  return true;
}

bool DPA_stream_fillWrite( DPA_stream_t* stream, char constant, size_t size ){
  if(DPA_ringbuffer_full(&stream->buffer_buffer->super))
    return false;
  DPA_bufferInfo_t entry = {0};
  entry.type = BUFFER_CONSTANT;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.constant = constant;
  DPA_RINGBUFFER_PUT( stream->buffer_buffer, entry );
  return true;
}

#ifdef __FLASH
bool DPA_stream_progmemWrite( DPA_stream_t* stream, const flash void* p, size_t size ){
  if(DPA_ringbuffer_full(&stream->buffer_buffer->super))
    return false;
  DPA_bufferInfo_t entry = {0};
  entry.type = BUFFER_FLASH;
  entry.range.size = size;
  entry.range.offset = 0;
  entry.fptr = p;
  DPA_RINGBUFFER_PUT( stream->buffer_buffer, entry );
  return true;
}
#endif

static inline size_t stream_read_from_buffer(
  const DPA_stream_t* stream,
  DPA_bufferInfo_t* info,
  void* p,
  size_t max_size
){
  size_t size = info->range.size - info->range.offset;
  size_t amount = size < max_size ? size : max_size;
  size_t read = DPA_ringbuffer_read(&stream->buffer->super,p,amount);
  info->range.offset += read;
  if( read != amount ){
    info->range.size = read + info->range.offset;
    DPA_LOG(
      "\x1b[31mBUG: Ringbuffer doesn't contain es much data as expected. "
      "This may happen if the streambuffers are modified without using the stream API or "
      "by using the stream API in unintended ways.\x1b[39m\n"
    );
  }
  return read;
}

static inline size_t stream_read_from_array(
  const DPA_stream_t* stream,
  DPA_bufferInfo_t* info,
  void* p,
  size_t max_size
){
  (void)stream;
  size_t size = info->range.size - info->range.offset;
  size_t n = size < max_size ? size : max_size;
  memcpy(p,(const char*)info->cptr+info->range.offset,n);
  info->range.offset += n;
  return n;
}

static inline size_t stream_read_constant(
  const DPA_stream_t* stream,
  DPA_bufferInfo_t* info,
  void* p,
  size_t max_size
){
  (void)stream;
  size_t size = info->range.size - info->range.offset;
  size_t n = size < max_size ? size : max_size;
  memset(p,info->constant,n);
  info->range.offset += n;
  return n;
}


#ifdef __FLASH
static inline size_t stream_read_from_flash(
  const DPA_stream_t* stream,
  DPA_bufferInfo_t* info,
  void* p,
  size_t max_size
){
  (void)stream;
  size_t size = info->range.size - info->range.offset;
  size_t n = size < max_size ? size : max_size;
  DPA_progmem_memcpy(p,(const flash char*)info->fptr+info->range.offset,n);
  info->range.offset += n;
  return n;
}
#endif

size_t DPA_stream_read( DPA_stream_t* stream, void* p, size_t max_size ){
  size_t n = max_size;
  while( n && !DPA_ringbuffer_eof(&stream->buffer_buffer->super) ){
    DPA_bufferInfo_t* info = DPA_ringbuffer_begin(stream->buffer_buffer);
    size_t size = 0;
    switch( info->type ){
      case BUFFER_BUFFER:
        size = stream_read_from_buffer(stream,info,p,n);
      break;
      case BUFFER_ARRAY:
        size = stream_read_from_array(stream,info,p,n);
      break;
      case BUFFER_CONSTANT:
        size += stream_read_constant(stream,info,p,n);
      break;
#ifdef __FLASH
      case BUFFER_FLASH:
        size = stream_read_from_flash(stream,info,p,n);
      break;
#endif
      default: return max_size - n;
    }
    n -= size;
    p = (char*)p+size;
    if( info->range.offset >= info->range.size )
      DPA_ringbuffer_increment_read(&stream->buffer_buffer->super);
  }
  return max_size - n;
}

size_t DPA_stream_seek( DPA_stream_t* stream, size_t size ){
  while( size ){
    DPA_bufferInfo_t* info = DPA_ringbuffer_begin(stream->buffer_buffer);
    if( info->range.size - info->range.offset <= size ){
      if( info->type == BUFFER_BUFFER )
        DPA_ringbuffer_skip_read( &stream->buffer->super, info->range.size - info->range.offset );
      size -= info->range.size - info->range.offset;
      info->range.offset = info->range.size;
      if( DPA_ringbuffer_eof(&stream->buffer_buffer->super) )
        break;
      DPA_ringbuffer_increment_read( &stream->buffer_buffer->super );
    }else{
      if( info->type == BUFFER_BUFFER )
        DPA_ringbuffer_skip_read( &stream->buffer->super, size );
      info->range.offset += size;
      size = 0;
    }
  }
  return size;
}

size_t DPA_stream_rewind( DPA_stream_t* stream, size_t size ){
  if( DPA_ringbuffer_eof(stream->buffer_buffer) )
    DPA_ringbuffer_decrement_read( &stream->buffer_buffer->super );
  while( size ){
    DPA_bufferInfo_t* info = DPA_ringbuffer_begin(stream->buffer_buffer);
    if( info->range.offset <= size ){
      if( info->type == BUFFER_BUFFER )
        DPA_ringbuffer_rewind_read( &stream->buffer->super, info->range.offset );
      size -= info->range.offset;
      info->range.offset = 0;
      if( DPA_ringbuffer_full(&stream->buffer_buffer->super) )
        break;
      DPA_ringbuffer_decrement_read( &stream->buffer_buffer->super );
    }else{
      if( info->type == BUFFER_BUFFER )
        DPA_ringbuffer_rewind_read( &stream->buffer->super, size );
      info->range.offset -= size;
      size = 0;
    }
  }
  return size;
}

/*********************************************************************************************
* A stream can contain more than SIZE_MAX bytes of datas which would lead to an overflow if  *
* not preventet. I solved this problem by Specifying an upper limit for the return value and *
* an returning an information if there would have been more datas.                           *
* This function is slow, avoid if possible                                                   *
**********************************************************************************************/
size_t DPA_stream_getLength( const DPA_stream_t* stream, size_t max_ret, bool* has_more ){
  DPA_buffer_ringbuffer_t tmp = *stream->buffer_buffer;
  size_t n = 0;
  while( !DPA_ringbuffer_eof(&tmp.super) ){
    const DPA_buffer_range_t range = DPA_RINGBUFFER_GET( &tmp ).range;
    size_t s = range.size > range.offset ? range.size - range.offset : 0;
    if( s + n < n || s + n > max_ret ){
      if(has_more)
        *has_more = true;
      return max_ret;
    }
    n += s;
  }
  if(has_more)
    *has_more = false;
  return n;
}

size_t DPA_stream_raw_getLength( const DPA_stream_raw_t* stream, size_t max_ret, bool* has_more ){
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
  if(has_more)
    *has_more = false;
  return n;
}
