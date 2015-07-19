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
  return (cacheEntry_t**)entry;
}

void accessCache( cacheEntry_t** entry, cacheAccessFunc_t func ){
  (void)entry;
  (void)func;
}

void removeFromCache( cacheEntry_t** entry ){
  (void)entry;
}
