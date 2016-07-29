#include <string.h>
#include <DPA/UCS/utils.h>
#include <DPA/UCS/ringbuffer.h>

size_t DPAUCS_ringbuffer_increment_read( DPAUCS_ringbuffer_base_t* ringbuffer ){
  if(!ringbuffer->range.size)
    return ringbuffer->range.offset - ringbuffer->inverse;
  ringbuffer->range.size--;
  size_t res;
  if(ringbuffer->inverse){
    res = --ringbuffer->range.offset;
    if(!res)
      ringbuffer->range.offset = ringbuffer->size;
  }else{
    res = ringbuffer->range.offset++;
    if( res+1 >= ringbuffer->size )
      ringbuffer->range.offset = 0;
  }
  return res;
}

static inline size_t DPAUCS_ringbuffer_end( DPAUCS_ringbuffer_base_t* ringbuffer ){
  size_t offset = ringbuffer->range.offset;
  size_t size = ringbuffer->range.size;
  if(!size){
    return ringbuffer->range.offset + ( ringbuffer->inverse ? -1 : 1 );
  }else if( ringbuffer->inverse ){
    if( offset <= size ){
      return offset - size;
    }else{
      return ringbuffer->size - size + offset;
    }
  }else{
    size_t remaining = ringbuffer->size - offset;
    if( remaining >= size ){
      return offset + size;
    }else{
      return size - remaining;
    }
  }
}

size_t DPAUCS_ringbuffer_increment_write( DPAUCS_ringbuffer_base_t* ringbuffer ){
  if( ringbuffer->range.size != ringbuffer->size )
    ringbuffer->range.size++;
  return DPAUCS_ringbuffer_end(ringbuffer) - ringbuffer->inverse;
}

size_t DPAUCS_ringbuffer_decrement_read( DPAUCS_ringbuffer_base_t* ringbuffer ){
  if( ringbuffer->range.size != ringbuffer->size )
    ringbuffer->range.size++;
  if( ringbuffer->inverse ){
    if( ringbuffer->range.offset >= ringbuffer->range.size )
      ringbuffer->range.offset = 0;
    return ringbuffer->range.offset++;
  }else{
    if( !ringbuffer->range.offset )
      ringbuffer->range.offset = ringbuffer->size;
    return --ringbuffer->range.offset;
  }
}

size_t DPAUCS_ringbuffer_decrement_write( DPAUCS_ringbuffer_base_t* ringbuffer ){
  size_t res = DPAUCS_ringbuffer_end(ringbuffer) - ringbuffer->inverse;;
  if( ringbuffer->range.size )
    ringbuffer->range.size--;
  return res;
}

void DPAUCS_ringbuffer_reverse( DPAUCS_ringbuffer_base_t* ringbuffer ){
  ringbuffer->range.offset = DPAUCS_ringbuffer_end(ringbuffer);
  ringbuffer->inverse = !ringbuffer->inverse;
}

size_t DPAUCS_ringbuffer_read( DPAUCS_ringbuffer_base_t* ringbuffer, void* destination, size_t size ){
  const size_t ts = ringbuffer->type_size;
  const size_t offset = ringbuffer->range.offset;
  const size_t fullsize = ringbuffer->size;
  const size_t count = ringbuffer->range.size;
  if( size > count )
    size = count;
  if( ringbuffer->inverse ){
    size_t n = size;
    size_t off = 0;
    if( offset < size ){
      n = offset;
      off = size-offset;
      size_t newoff = fullsize-off;
      memrcpy( ts, destination, ((char*)ringbuffer->buffer)+newoff*ts, off );
      ringbuffer->range.offset = newoff;
    }else{
      ringbuffer->range.offset -= size;
    }
    memrcpy( ts, ((char*)destination)+off*ts, ((char*)ringbuffer->buffer)+(offset-n)*ts, n );
  }else{
    size_t remaining = fullsize - offset;
    size_t n = size;
    if( remaining < size ){
      n = remaining;
      size_t amount = size-remaining;
      memcpy( ((char*)destination)+amount*ts, ringbuffer->buffer, amount*ts );
      ringbuffer->range.offset = amount;
    }else{
      ringbuffer->range.offset += size;
    }
    memcpy( destination, ((char*)ringbuffer->buffer) + offset * ts, n * ts );
  }
  ringbuffer->range.size -= size;
  return size;
}

size_t DPAUCS_ringbuffer_write( DPAUCS_ringbuffer_base_t* ringbuffer, const void* source, size_t size ){
  const size_t ts = ringbuffer->type_size;
  const size_t offset = DPAUCS_ringbuffer_end(ringbuffer);
  const size_t fullsize = ringbuffer->size;
  const size_t count = fullsize-ringbuffer->range.size;
  if( size > count )
    size = count;
  if( ringbuffer->inverse ){
    size_t n = size;
    size_t off = 0;
    if( offset < size ){
      n = offset;
      off = size-offset;
      size_t newoff = fullsize-off;
      memrcpy( ts, ((char*)ringbuffer->buffer)+newoff*ts, source, off );
    }
    memrcpy( ts, ((char*)ringbuffer->buffer)+(offset-n)*ts, ((const char*)source)+off*ts, n );
  }else{
    size_t remaining = fullsize - offset;
    size_t n = size;
    if( remaining < size ){
      n = remaining;
      size_t amount = size-remaining;
      memcpy( ringbuffer->buffer, ((const char*)source)+amount*ts, amount*ts );
    }
    memcpy( ((char*)ringbuffer->buffer) + offset * ts, source, n * ts );
  }
  ringbuffer->range.size += size;
  return size;
}

/* TODO
size_t DPAUCS_ringbuffer_skip_read( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size ){
  
}

size_t DPAUCS_ringbuffer_rewind_read( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size ){
  
}

size_t DPAUCS_ringbuffer_skip_write( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size ){
  
}

size_t DPAUCS_ringbuffer_rewind_write( DPAUCS_ringbuffer_base_t* ringbuffer, size_t size ){
  
}

*/

void DPAUCS_ringbuffer_reset( DPAUCS_ringbuffer_base_t* ringbuffer ){
  ringbuffer->range.size = 0;
  ringbuffer->range.offset = ringbuffer->inverse ? ringbuffer->size : 0;
}

