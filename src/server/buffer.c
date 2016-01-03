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

