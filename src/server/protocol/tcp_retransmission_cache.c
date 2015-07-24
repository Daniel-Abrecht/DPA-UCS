#include <stdint.h>
#include <string.h>
#include <mempool.h>
#include <protocol/tcp_retransmission_cache.h>

static char buffer[TCP_RETRANSMISSION_CACHE_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
static void* cacheEntries[TCP_RETRANSMISSION_CACHE_MAX_ENTRIES];

static inline size_t ptrcount( void* x ){
  void** y = x;
  size_t i = 0;
  while( *y++ ) i++;
  return i;
}

cacheEntry_t** addToCache( DPAUCS_tcp_transmission_t* t, transmissionControlBlock_t** tcb, uint16_t flags ){
  cacheEntry_t e = {
    .tcbBufferSize = ( ptrcount( tcb ) + 1 ) * sizeof(tcb),
    .charBufferSize = BUFFER_SIZE( t->stream->buffer ) * sizeof(char),
    .streamBufferSize = BUFFER_SIZE( t->stream->buffer_buffer ) * sizeof(bufferInfo_t),
    .flags = flags
  };
  size_t fullSize = sizeof(e)
                  + e.tcbBufferSize
                  + e.charBufferSize
                  + e.streamBufferSize;
  void** entry = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
  while( --entry >= cacheEntries && *entry );
  if(entry < cacheEntries)
    return 0;
  DPAUCS_mempool_alloc( &mempool, entry, fullSize );
  if(!*entry)
    return 0;

  {
    DPAUCS_stream_offsetStorage_t sros;
    DPAUCS_stream_saveReadOffset( &sros, t->stream );

    char* emem = *entry;
    memcpy( emem, &e , sizeof(e)       ); emem += sizeof(e);
    memcpy( emem, tcb, e.tcbBufferSize ); emem += e.tcbBufferSize;

    {
      uchar_buffer_t* cb = t->stream->buffer;
      while(!BUFFER_EOF( cb ))
        *emem++ = BUFFER_GET( cb );
    }
    {
      buffer_buffer_t* bb = t->stream->buffer_buffer;
      bufferInfo_t* bmem = (bufferInfo_t*)emem;
      while(!BUFFER_EOF( bb ))
        *bmem++ = BUFFER_GET( bb );
      emem += e.streamBufferSize;
    }

    DPAUCS_stream_restoreReadOffset( t->stream, &sros );
  }

  return (cacheEntry_t**)entry;
}

void removeFromCache( cacheEntry_t** entry ){
  DPAUCS_mempool_free( &mempool, (void**)entry );
}

void tcbRemovationHandler( transmissionControlBlock_t* tcb ){
  void** entry = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
  while( --entry >= cacheEntries ){
    if(!*entry)
      continue;
    transmissionControlBlock_t** it = (transmissionControlBlock_t**)(void*)( ((cacheEntry_t*)*entry) + 1 );
    size_t n = ((cacheEntry_t*)*entry)->tcbBufferSize;
    size_t m = 0;
    for( ; *it; it++ ){
      n--;
      if( *it != tcb ){
        m++;
        continue;
      }
      memcpy( it, it+1, n );
      it--;
    }
    if(!m) // no tcb's left, remove entry
      removeFromCache( (cacheEntry_t**)entry );
  }
}
