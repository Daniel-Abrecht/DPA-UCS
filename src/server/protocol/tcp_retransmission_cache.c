#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <DPA/UCS/logger.h>
#include <DPA/UCS/mempool.h>
#include <DPA/UCS/protocol/tcp_retransmission_cache.h>

static char buffer[TCP_RETRANSMISSION_CACHE_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
static tcp_cacheEntry_t* cacheEntries[TCP_RETRANSMISSION_CACHE_MAX_ENTRIES];

typedef struct {
  transmissionControlBlock_t** TCBs;
  uint16_t* flags;
  unsigned char* charBuffer;
  bufferInfo_t* streamBuffer;
} tcp_cache_entryInfo_t;

static inline void GCC_BUGFIX_50925 getEntryInfo( tcp_cache_entryInfo_t* res, tcp_cacheEntry_t* entry ){
  char* emem = (char*)entry;
  size_t count = entry->count;
  size_t offset = DPAUCS_CALC_ALIGN_OFFSET( sizeof(tcp_cacheEntry_t), transmissionControlBlock_t* );
  res->TCBs = (void*)( emem + offset );
  offset = DPAUCS_CALC_ALIGN_OFFSET( offset + count * sizeof( transmissionControlBlock_t* ), uint16_t );
  res->flags = (void*)( emem + offset );
  offset = DPAUCS_CALC_ALIGN_OFFSET( offset + count * sizeof( uint16_t ), bufferInfo_t );
  res->streamBuffer = (void*)( emem + offset );
  offset += entry->streamBufferSize * sizeof( bufferInfo_t );
  res->charBuffer = (void*)( emem + offset );
}

bool tcp_addToCache( DPAUCS_tcp_transmission_t* t, unsigned count, transmissionControlBlock_t** tcb, uint16_t* flags ){

  tcp_cacheEntry_t e = {
    .count = count,
    .next = 0,
    .adelay = 0,
    .charBufferSize   = BUFFER_SIZE( t->stream->buffer        ),
    .streamBufferSize = BUFFER_SIZE( t->stream->buffer_buffer )
  };
  e.streamRealLength = DPAUCS_stream_getLength( t->stream, ~0, &e.streamIsLonger );

  size_t                             fullSize  = sizeof( e );
  const size_t tcb_offset          = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, transmissionControlBlock_t* );
                                     fullSize += count * sizeof( transmissionControlBlock_t* );
  const size_t flags_offset        = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, uint16_t );
                                     fullSize += count * sizeof( uint16_t );
  const size_t streamBuffer_offset = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, bufferInfo_t );
  const size_t charBuffer_offset   = fullSize += e.streamBufferSize * sizeof( bufferInfo_t );
                                     fullSize += e.charBufferSize;

  tcp_cacheEntry_t** entry = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
  while( --entry >= cacheEntries && *entry );
  if(entry < cacheEntries)
    return false;

  DPAUCS_mempool_alloc( &mempool, (void**)entry, fullSize );
  if( !*entry ) return false;

  char* emem = (char*)*entry;
  memcpy( emem               , &e   , sizeof(e)              );
  memcpy( emem + tcb_offset  , tcb  , count * sizeof(*tcb)   );
  memcpy( emem + flags_offset, flags, count * sizeof(*flags) );

  DPAUCS_stream_to_raw_buffer(
    t->stream,
    (void*)( emem + streamBuffer_offset ),
    e.streamBufferSize,
    (void*)( emem +   charBuffer_offset ),
    e.charBufferSize
  );

  for( transmissionControlBlock_t *it=*tcb, *end=it+count; it<end; it++ ){
    if( (!it->cache.first) != (it->SND.NXT==it->SND.UNA) ){
      // This should never happen
      DPAUCS_LOG( "\x1b[31mCritical BUG: %s\x1b[39m\n",
        it->cache.first ? "A cache entry exists, but any octet is already acknowledged!"
                        : "Some octets aren't yet acknowledged, but they aren't cached!"
      );
    }
    if( !it->cache.first ){
      it->cache.last = it->cache.first = (tcp_cacheEntry_t**)entry;
      it->cache.first_SEQ = it->SND.NXT;
    }else{
      it->cache.last = (*it->cache.last)->next = (tcp_cacheEntry_t**)entry;
    }
  }

  return true;
}

void removeFromCache( tcp_cacheEntry_t** entry ){
  DPAUCS_mempool_free( &mempool, (void**)entry );
}

bool tcp_cleanupCacheEntryCheckTCB( tcp_cacheEntry_t** entry, tcp_cache_entryInfo_t* info, unsigned tcb_index ){
  tcp_cacheEntry_t* e = *entry;
  tcp_cache_entryInfo_t info_tmp;
  if( !info ){
    info = &info_tmp;
    getEntryInfo( info, e );
  }
  transmissionControlBlock_t* tcb = info->TCBs[tcb_index];
  if( !tcb ) return true;
  if( tcb->cache.first != entry )
    return false;
  uint32_t acknowledged_octets = tcb->SND.UNA - tcb->cache.first_SEQ;
  // check if all octets may have been acknowledged
  if( acknowledged_octets && acknowledged_octets-e->streamIsLonger >= e->streamRealLength ){
    // update streamlength info if necessary
    //if( e->streamIsLonger )
      //e->streamRealLength = DPAUCS_stream_getLength( t->stream, ~0, &e->streamIsLonger );
    // check if really all octets have been acknowledged
    if( acknowledged_octets && acknowledged_octets-e->streamIsLonger >= e->streamRealLength ){
      // remove tcb from entry and entry from tcb
      info->TCBs[tcb_index] = 0;
      tcb->cache.first = e->next;
      if( tcb->cache.last == entry )
        tcb->cache.last = tcb->cache.first;
      for( size_t i=tcb_index+1, n=e->count; i < n; i++ ){
        if( !info->TCBs[i] ) break;
        info->TCBs[i-1] = info->TCBs[i];
        info->flags[i-1] = info->flags[i];
      }
    }
  }
  if(!*info->TCBs){ // no tcb's left, remove entry
    removeFromCache( entry );
    return true;
  }
  return false;
}

void tcp_cleanupCache(){
/*  tcp_cacheEntry_t** it = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
  while( --it >= cacheEntries )
    if( it )
      tcp_cleanupCacheEntry( it );*/
}

static void retransmit( tcp_cache_entryInfo_t* ei ){
  (void)ei;
  printf("retransmit\n");
}

// TODO: Make this dynamic, it's required by RFC 793 Page 41
#define RETRANSMISSION_INTERVAL 500

static bool do_retransmissions( void** entry, void* unused ){
  (void)unused;
  tcp_cacheEntry_t* e = *entry;
  tcp_cache_entryInfo_t info;
  
  getEntryInfo( &info, e );
  if(!adelay_done( &e->adelay, RETRANSMISSION_INTERVAL ))
    return true;
  retransmit( &info );
  return true;
}

void tcp_retransmission_cache_do_retransmissions( void ){
  DPAUCS_mempool_each( &mempool, do_retransmissions, 0 );
}
