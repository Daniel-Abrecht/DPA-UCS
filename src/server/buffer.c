#include <DPA/UCS/buffer.h>

size_t DPAUCS_buffer_increment( DPAUCS_ringbuffer_state_t* state, bool readOrWrite ){
  size_t* offset = state->inverse == readOrWrite ? &state->offset.end : &state->offset.start;
  size_t ret = *offset;
  size_t size = DPAUCS_buffer_size(state);
  if( readOrWrite ){
    if( size == 1 )
      state->empty = true;
    if(!size)
      return state->size;
  }else{
    state->empty = false;
  }
  if( state->inverse ){
    if( ret ){
      --*offset;
    }else{
      *offset = state->size-1;
    }
  }else{
    if( ++*offset == state->size )
      *offset = 0;
  }
  return ret;
}

size_t DPAUCS_buffer_size( const DPAUCS_ringbuffer_state_t* state ){
  if( state->empty )
    return 0;
  return state->offset.end > state->offset.start
   ? state->offset.end - state->offset.start
   : state->size - state->offset.end + state->offset.start
  ;
}

void DPAUCS_buffer_reset( DPAUCS_ringbuffer_state_t* state ){
  state->offset.start = state->offset.end = state->inverse ? state->size - 1 : 0;
}

// Returns diffrence between offsets
size_t DPAUCS_buffer_getOffsetDifference(
  bool* bigger,
  DPAUCS_ringbuffer_state_t* state,
  size_t offset,
  bool readOrWrite
){
  if(!~offset){
    // If the other stream was empty, which is the case when ~offset == 0, it doesn't have a valid offset but a size of 0.
    // Therefore, the difference between both offsets is the size of the first stream
    *bigger = false;
    return DPAUCS_buffer_size( state );
  }
  bool startOrEnd = state->inverse != readOrWrite;
  // Unchanged other offset
  size_t offset2 = (!startOrEnd) ? state->offset.start : state->offset.end;
  size_t start = startOrEnd ? offset  : offset2;
  size_t end   = startOrEnd ? offset2 : offset ;
  size_t size  = DPAUCS_buffer_size( state );
  size_t size2 = end > start ? end - start : state->size - end + start;
  if( size2 > size ){
    *bigger = true;
    return size2 - size;
  }else{
    *bigger = false;
    return size - size2;
  }
}


void DPAUCS_buffer_skip( DPAUCS_ringbuffer_state_t* state, size_t x, bool reverse ){
  if(!x)
    return;
  size_t* offset_read = state->inverse ? &state->offset.end : &state->offset.start;
  size_t size = DPAUCS_buffer_size(state);
  if( reverse ){
    if( state->size - size <= x ){
      x = state->size - size;
      state->empty = true;
    }
  }else{
    if( size < x )
      x = size;
    state->empty = false;
  }
  if( reverse != state->inverse ){
    size_t p = *offset_read;
    if( p < x ){
      *offset_read  = state->size - x + p;
    }else{
      *offset_read -= x;
    }
  }else{
    size_t p = state->size - *offset_read;
    if( p <= x ){
      *offset_read  = x - p;
    }else{
      *offset_read += x;
    }
  }
}

size_t DPAUCS_buffer_get_offset( DPAUCS_ringbuffer_state_t* state, bool readOrWrite ){
  if(state->empty)
    return (size_t)~0;
  return state->inverse != readOrWrite ? state->offset.start : state->offset.end;
}

void DPAUCS_buffer_set_offset( DPAUCS_ringbuffer_state_t* state, bool readOrWrite, size_t offset ){
  if(!~offset){
    state->empty = true;
    state->offset.start = state->offset.end = state->inverse ? state->size - 1 : 0;
  }else{
    if( state->inverse != readOrWrite ){
      state->offset.start = offset;
    }else{
      state->offset.end = offset;
    }
  }
}

