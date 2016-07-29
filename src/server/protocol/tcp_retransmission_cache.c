#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <DPA/UCS/logger.h>
#include <DPA/UCS/mempool.h>
#include <DPA/UCS/protocol/tcp.h>
#include <DPA/UCS/protocol/tcp_retransmission_cache.h>

static char buffer[TCP_RETRANSMISSION_CACHE_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
static void* cacheEntries[TCP_RETRANSMISSION_CACHE_MAX_ENTRIES];

#define DCE( X ) ((DPAUCS_tcp_cacheEntry_t*)((char*)(X)+*(size_t*)(X)))

typedef struct tcp_cache_entry_tcb_entry {
  DPAUCS_transmissionControlBlock_t* tcb;
  void** next;
  uint16_t flags;
} tcp_cache_entry_tcb_entry_t;

typedef struct {
  unsigned char* charBuffer;
  DPAUCS_bufferInfo_t* bufferBuffer;
  tcp_cache_entry_tcb_entry_t* tcb_list;
} tcp_cache_entryInfo_t;

static inline void GCC_BUGFIX_50925 getEntryInfo( tcp_cache_entryInfo_t* res, void* off ){
  char* emem = (char*)off;
  DPAUCS_tcp_cacheEntry_t* entry = (DPAUCS_tcp_cacheEntry_t*)( emem + *(size_t*)off );
  size_t count = entry->count;
  size_t offset = DPAUCS_CALC_PREV_ALIGN_OFFSET( *(size_t*)off - entry->bufferBufferSize * sizeof( DPAUCS_bufferInfo_t ), DPAUCS_bufferInfo_t );
  res->bufferBuffer = (void*)( emem + offset );
  offset  = DPAUCS_CALC_ALIGN_OFFSET( sizeof(*entry) + *(size_t*)off, tcp_cache_entry_tcb_entry_t );
  res->tcb_list = (void*)( emem + offset );
  offset += count * sizeof( tcp_cache_entry_tcb_entry_t );
  res->charBuffer = (void*)( emem + offset );
}

bool tcp_addToCache( DPAUCS_tcp_transmission_t* t, unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags ){

  DPAUCS_tcp_cacheEntry_t e = {
    .count = count,
    .charBufferSize   = DPAUCS_ringbuffer_size( t->stream->buffer        ),
    .bufferBufferSize = DPAUCS_ringbuffer_size( t->stream->buffer_buffer ),
  };
  e.streamRealLength = DPAUCS_stream_getLength( t->stream, ~0, &e.streamIsLonger );

  void** entry;

  for( unsigned i=count; i--; ){
    DPAUCS_transmissionControlBlock_t* it = tcb[i];
    if( flags[i] & TCP_FLAG_SYN )
      it->cache.flags.SYN = true;
    if( flags[i] & TCP_FLAG_FIN )
      it->cache.flags.FIN = true;
    if( !it->cache.first )
      it->cache.first_SEQ = it->SND.NXT;
  }

  if( e.streamRealLength ){

    size_t                             fullSize  = sizeof( size_t );
    const size_t bufferBuffer_offset = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, DPAUCS_bufferInfo_t );
                                       fullSize += e.bufferBufferSize * sizeof( DPAUCS_bufferInfo_t );
    const size_t entry_offset        = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, DPAUCS_tcp_cacheEntry_t );
                                       fullSize += sizeof( DPAUCS_tcp_cacheEntry_t );
    const size_t tcb_list_offset     = fullSize  = DPAUCS_CALC_ALIGN_OFFSET( fullSize, tcp_cache_entry_tcb_entry_t );
    const size_t charBuffer_offset   = fullSize += count * sizeof( DPAUCS_transmissionControlBlock_t* );
                                       fullSize += e.charBufferSize;

    DPAUCS_LOG(
      "tcp_addToCache: count %u, Cache entry size: %llu, Stream length: %llu\n", count,
      (unsigned long long)fullSize, (unsigned long long)e.streamRealLength
    );

    entry = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
    while( --entry >= cacheEntries && *entry );
    if(entry < cacheEntries)
      return false;

    DPAUCS_mempool_alloc( &mempool, (void**)entry, fullSize );
    if( !*entry ) return false;

    char* emem = (char*)*entry;
    *(size_t*)*entry = entry_offset;
    *(DPAUCS_tcp_cacheEntry_t*)( emem + entry_offset ) = e;
    tcp_cache_entry_tcb_entry_t* tcb_list = (tcp_cache_entry_tcb_entry_t*)( emem + tcb_list_offset );

    for( unsigned i=count; i--; ){
      tcb_list[i] = (tcp_cache_entry_tcb_entry_t){
        .tcb  = tcb[i],
        .next = 0,
        .flags = flags[i]
      };
    }

    DPAUCS_stream_raw_t raw_stream = {
      .bufferBufferSize = e.bufferBufferSize,
      .charBufferSize   = e.charBufferSize,
      .bufferBuffer     = (void*)( emem + bufferBuffer_offset ),
      .charBuffer       = (void*)( emem +   charBuffer_offset )
    };

    DPAUCS_stream_to_raw_buffer( t->stream, &raw_stream );

    for( unsigned i=count; i--; ){
      DPAUCS_transmissionControlBlock_t* it = tcb[i];
      if( (!it->cache.first) != (it->SND.NXT==it->SND.UNA) ){
        // This should never happen
        DPAUCS_LOG( "\x1b[31mCritical BUG: %s\x1b[39m\n",
          it->cache.first ? "A cache entry exists, but any octet is already acknowledged!"
                          : "Some octets aren't yet acknowledged, but they aren't cached!"
        );
      }
      if( !it->cache.first ){
        it->cache.last_transmission = 0;
        it->cache.last = it->cache.first = entry;
      }else{
        tcp_cache_entryInfo_t inf;
        DPAUCS_tcp_cacheEntry_t*const e = DCE( *it->cache.last );
        getEntryInfo( &inf, e );
        tcp_cache_entry_tcb_entry_t* list = inf.tcb_list;
        for( tcp_cache_entry_tcb_entry_t *it2=list,*end=list+e->count; it2 < end && it2->tcb; it2++ ){
          if( it2->tcb == it ){
            it->cache.last = it2->next = entry;
            break;
          }
        }
      }
    }

  }

  return true;
}

static void removeFromCache( void** entry ){
  DPAUCS_mempool_free( &mempool, entry );
}

static bool tcp_cleanupCacheEntryCheckTCB( DPAUCS_transmissionControlBlock_t* tcb ){
  if( !tcb || !tcb->cache.first || !*tcb->cache.first )
    return false;
  void**const entry = tcb->cache.first;
  DPAUCS_tcp_cacheEntry_t*const e = DCE(*entry);
  tcp_cache_entryInfo_t info;
  getEntryInfo( &info, e );
  unsigned tcb_index = e->count;
  while( tcb_index-- )
    if( info.tcb_list[tcb_index].tcb == tcb )
      break;
  if( !~tcb_index )
    return false;
  uint32_t acknowledged_octets = tcb->SND.UNA - tcb->cache.first_SEQ;
  bool ret = false;
  // check if all octets may have been acknowledged
  if( acknowledged_octets && acknowledged_octets-e->streamIsLonger >= e->streamRealLength ){
    // update streamlength info if necessary
    if( e->streamIsLonger ){
      DPAUCS_stream_raw_t raw_stream = {
        .bufferBufferSize = e->bufferBufferSize,
        .charBufferSize   = e->charBufferSize,
//        .bufferBuffer     = info.bufferBuffer,
        .charBuffer       = info.charBuffer
      };
      e->streamRealLength = DPAUCS_stream_raw_getLength( &raw_stream, ~0, &e->streamIsLonger );
    }
    // check if really all octets have been acknowledged
    if( acknowledged_octets && acknowledged_octets-e->streamIsLonger >= e->streamRealLength ){
      // remove tcb from entry and entry from tcb
      ret = true;
      info.tcb_list[tcb_index].tcb = 0;
      tcb->cache.first = info.tcb_list[tcb_index].next;
      tcb->cache.first_SEQ = e->streamRealLength;
      if( !tcb->cache.first )
        tcb->cache.last = 0;
      for( size_t i=tcb_index+1, n=e->count; i < n; i++ ){
        if( !info.tcb_list[i].tcb ) break;
        info.tcb_list[i-1] = info.tcb_list[i];
      }
    }
  }
  if(!info.tcb_list->tcb){ // no tcb's left, remove entry
    removeFromCache( entry );
    return true;
  }
  return ret;
}

void tcp_cacheCleanupTCB( DPAUCS_transmissionControlBlock_t* tcb ){
  while( tcp_cleanupCacheEntryCheckTCB( tcb ) );
}

static void tcp_cacheDiscardEntryOctets( void**const entry, uint32_t size ){
  tcp_cache_entryInfo_t info;
  DPAUCS_tcp_cacheEntry_t*const e = DCE(*entry);
  getEntryInfo( &info, e );
  DPAUCS_stream_raw_t raw_stream = {
    .bufferBufferSize = e->bufferBufferSize,
    .charBufferSize   = e->charBufferSize,
//        .bufferBuffer     = info.bufferBuffer,
    .charBuffer       = info.charBuffer
  };
  DPAUCS_raw_stream_truncate( &raw_stream, size );
//  DPAUCS_mempool_realloc( mempool, (void**)entry,  );
  (void)entry;
  (void)size;
}

static void tcp_cleanupEntry( void**const entry ){

  DPAUCS_tcp_cacheEntry_t*const e = DCE(*entry);
  tcp_cache_entryInfo_t info;
  getEntryInfo( &info, e );

  uint32_t acknowledged_octets_min = ~0;
  unsigned n = e->count;
  for( unsigned i = n; i--; ){
    DPAUCS_transmissionControlBlock_t* tcb = info.tcb_list[i].tcb;
    uint32_t acknowledged_octets = tcb->SND.UNA - tcb->cache.first_SEQ;
    if( acknowledged_octets_min > acknowledged_octets )
      acknowledged_octets_min = acknowledged_octets;
  }

  if( acknowledged_octets_min )
  for( unsigned i = n; i--; ){
    DPAUCS_transmissionControlBlock_t* tcb = info.tcb_list[i].tcb;
    tcb->cache.first_SEQ += acknowledged_octets_min;
  }
  tcp_cacheDiscardEntryOctets( entry, acknowledged_octets_min );

}

void tcp_cleanupCache( void ){
  DPAUCS_transmissionControlBlock_t* start = DPAUCS_transmissionControlBlocks;
  DPAUCS_transmissionControlBlock_t* end = DPAUCS_transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( DPAUCS_transmissionControlBlock_t* it=start; it<end; it++ ){
    if( it->state == TCP_CLOSED_STATE
     || it->state == TCP_SYN_RCVD_STATE
     || it->state == TCP_TIME_WAIT_STATE
     || !it->cache.first
    ) continue;
    tcp_cleanupEntry( it->cache.first );
  }
}

#define RETRANSMISSION_INTERVAL AD_SEC

void tcp_retransmission_cache_do_retransmissions( void ){
  DPAUCS_transmissionControlBlock_t* start = DPAUCS_transmissionControlBlocks;
  DPAUCS_transmissionControlBlock_t* end = DPAUCS_transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( DPAUCS_transmissionControlBlock_t* it=start; it<end; it++ ){
    if( it->state == TCP_CLOSED_STATE
     || it->state == TCP_TIME_WAIT_STATE
     || ( it->cache.last_transmission
     && !adelay_done( &it->cache.last_transmission, RETRANSMISSION_INTERVAL ) )
    ) continue;
    adelay_start( &it->cache.last_transmission );

    DPAUCS_LOG("Retransmit entry for tcb %u\n", (unsigned)(it-DPAUCS_transmissionControlBlocks) );

    uint16_t flags = TCP_FLAG_ACK;

    DPAUCS_tcp_t tcp;
    DPAUCS_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
    DPAUCS_stream_referenceWrite( stream, &tcp, sizeof(tcp) );

    size_t size = 0;

    if( it->cache.first ){
      
    }

    if( it->cache.flags.SYN )
      flags |= TCP_FLAG_SYN;
    if( it->cache.flags.FIN )
      flags |= TCP_FLAG_FIN;

    DPAUCS_tcp_transmit( stream, &tcp, it, flags, size, it->cache.first_SEQ );

    DPAUCS_layer3_destroyTransmissionStream( stream );

  }
}
