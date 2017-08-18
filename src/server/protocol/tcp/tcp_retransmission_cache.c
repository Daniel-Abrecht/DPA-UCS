#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <DPA/utils/logger.h>
#include <DPA/utils/mempool.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/adelay.h>
#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/protocol/tcp/tcp_retransmission_cache.h>

#define RETRANSMISSION_INTERVAL AD_SEC * 2

#define DCE( X ) ((DPAUCS_tcp_cacheEntry_t*)((char*)(X)+*(size_t*)(X)))


static char buffer[TCP_RETRANSMISSION_CACHE_SIZE] = {0};
static DPA_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
static void* cacheEntries[TCP_RETRANSMISSION_CACHE_MAX_ENTRIES];


typedef struct packed tcp_cache_entry_tcb_entry {
  DPAUCS_transmissionControlBlock_t* tcb;
  void** next;
  uint16_t flags;
} tcp_cache_entry_tcb_entry_t;

typedef struct packed DPAUCS_tcp_cacheEntry {
  size_t charBufferSize;
  size_t bufferBufferSize;
  size_t streamRealLength;
  unsigned count;
  bool streamIsLonger;
  tcp_cache_entry_tcb_entry_t ctcb[];
} DPAUCS_tcp_cacheEntry_t;

typedef struct {
  unsigned char* charBuffer;
  DPA_bufferInfo_t* bufferBuffer;
  tcp_cache_entry_tcb_entry_t* tcb_list;
} tcp_cache_entryInfo_t;

static inline void GCC_BUGFIX_50925 getEntryInfo( tcp_cache_entryInfo_t* res, DPAUCS_tcp_cacheEntry_t* entry ){
  res->bufferBuffer = ((DPA_bufferInfo_t*)entry) - entry->bufferBufferSize;
  res->tcb_list = entry->ctcb;
  res->charBuffer = (unsigned char*)&res->tcb_list[entry->count];
}

bool tcp_addToCache( DPAUCS_tcp_transmission_t* t, unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags ){

  DPAUCS_tcp_cacheEntry_t e = {
    .count = count,
    .charBufferSize   = DPA_ringbuffer_size( t->stream->buffer        ),
    .bufferBufferSize = DPA_ringbuffer_size( t->stream->buffer_buffer ),
  };
  e.streamRealLength = DPA_stream_getLength( t->stream, ~0, &e.streamIsLonger );

  void** entry;

  for( unsigned i=count; i--; ){
    DPAUCS_transmissionControlBlock_t* it = tcb[i];
    if( flags[i] & TCP_FLAG_SYN )
      it->cache.flags.SYN = true;
    if( it->RCV.UNA != it->RCV.NXT ){
      it->cache.flags.ACK = true;
      it->cache.flags.need_ACK = true;
      it->RCV.UNA = it->RCV.NXT;
    }
    if( flags[i] & TCP_FLAG_FIN )
      it->cache.flags.FIN = true;
    if( !it->cache.first )
      it->cache.first_SEQ = it->SND.NXT;
  }

  if( e.streamRealLength ){

    size_t                             fullSize  = sizeof( size_t );
    const size_t bufferBuffer_offset = fullSize  = DPA_CALC_ALIGN_OFFSET( fullSize, DPA_bufferInfo_t );
    const size_t entry_offset        = fullSize += sizeof( DPA_bufferInfo_t[e.bufferBufferSize] );
    const size_t tcb_list_offset     = fullSize += sizeof( DPAUCS_tcp_cacheEntry_t );
    const size_t charBuffer_offset   = fullSize += sizeof( tcp_cache_entry_tcb_entry_t[count] );
                                       fullSize += e.charBufferSize;
    DPA_LOG(
      "tcp_addToCache: count %u, Cache entry size: %llu, Stream length: %llu\n", count,
      (unsigned long long)fullSize, (unsigned long long)e.streamRealLength
    );

    entry = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
    while( --entry >= cacheEntries && *entry );
    if(entry < cacheEntries)
      return false;

    DPA_mempool_alloc( &mempool, entry, fullSize );
    if( !*entry ) return false;
    DPA_LOG("%zu bytes requested, %zu allocated in entry %zu\n",fullSize,DPAUCS_MEMPOOL_SIZE(*entry),entry-cacheEntries);

    char* emem = (char*)*entry;
    *(size_t*)emem = entry_offset;
    *(DPAUCS_tcp_cacheEntry_t*)( emem + entry_offset ) = e;
    tcp_cache_entry_tcb_entry_t* tcb_list = (tcp_cache_entry_tcb_entry_t*)( emem + tcb_list_offset );

    for( unsigned i=count; i--; ){
      tcb_list[i] = (tcp_cache_entry_tcb_entry_t){
        .tcb  = tcb[i],
        .next = 0,
        .flags = flags[i] & ~(TCP_FLAG_SYN|TCP_FLAG_FIN)
      };
    }

    DPA_stream_raw_t raw_stream = {
      .bufferBufferSize = e.bufferBufferSize,
      .charBufferSize   = e.charBufferSize,
      .bufferBuffer     = (void*)( emem + bufferBuffer_offset ),
      .charBuffer       = (void*)( emem +   charBuffer_offset )
    };

    DPA_stream_to_raw_buffer( t->stream, &raw_stream );

    for( unsigned i=count; i--; ){
      DPAUCS_transmissionControlBlock_t* it = tcb[i];
      if( (!it->cache.first) != (it->SND.NXT==it->SND.UNA) ){
        // This should never happen
        DPA_LOG( "\x1b[31mCritical BUG: %s\x1b[39m\n",
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

  }else{
    for( size_t i=0; i<count; i++ )
      if( flags[i] & TCP_FLAG_RST )
        DPAUCS_tcp_transmit( 0, tcb[i], flags[i], 0, tcb[i]->SND.NXT );
  }

  return true;
}

static void removeFromCache( void** entry ){
  DPA_LOG("Entry %zu freed\n",entry-cacheEntries);
  DPA_mempool_free( &mempool, entry );
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
      DPA_stream_raw_t raw_stream = {
        .bufferBufferSize = e->bufferBufferSize,
        .charBufferSize   = e->charBufferSize,
        .bufferBuffer     = info.bufferBuffer,
        .charBuffer       = info.charBuffer
      };
      e->streamRealLength = DPA_stream_raw_getLength( &raw_stream, ~0, &e->streamIsLonger );
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
  return;
  while( tcp_cleanupCacheEntryCheckTCB( tcb ) );
}

static void tcp_cacheDiscardEntryOctets( void**const entry, uint32_t size ){
  tcp_cache_entryInfo_t info;
  DPAUCS_tcp_cacheEntry_t*const e = DCE(*entry);
  getEntryInfo( &info, e );
  DPA_stream_raw_t raw_stream = {
    .bufferBufferSize = e->bufferBufferSize,
    .charBufferSize   = e->charBufferSize,
    .bufferBuffer     = info.bufferBuffer,
    .charBuffer       = info.charBuffer
  };
  DPAUCS_raw_stream_truncate( &raw_stream, size );
//  DPA_mempool_realloc( mempool, (void**)entry,  );
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

struct tcp_retransmission_cache_do_retransmissions_sub_args {
  DPAUCS_transmissionControlBlock_t* tcb;
  uint16_t flags;
  size_t size;
  uint32_t SEQ;
};

static void tcp_retransmission_cache_do_retransmissions_sub( DPA_stream_t* stream, void* pargs ){
  struct tcp_retransmission_cache_do_retransmissions_sub_args* args = pargs;
  DPA_LOG( "tcp_retransmission_cache_do_retransmissions_sub: expected stream size: %zu, real stream size %zu\n", args->size, DPA_stream_getLength(stream,~0,0) );
  DPAUCS_tcp_transmit( stream, args->tcb, args->flags, args->size, args->SEQ );
}

void tcp_retransmission_cache_do_retransmissions( void ){
  DPAUCS_transmissionControlBlock_t* start = DPAUCS_transmissionControlBlocks;
  DPAUCS_transmissionControlBlock_t* end = DPAUCS_transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( DPAUCS_transmissionControlBlock_t* it=start; it<end; it++ ){
    if( it->cache.flags.need_ACK ){
      it->cache.flags.ACK = true;
    }else{
      if( it->state == TCP_CLOSED_STATE
      || it->state == TCP_TIME_WAIT_STATE
      ) continue;
      if( it->cache.last_transmission && !adelay_done( &it->cache.last_transmission, RETRANSMISSION_INTERVAL ) )
        continue;
    }
    adelay_start( &it->cache.last_transmission );
    it->cache.flags.need_ACK = false;

    DPA_LOG("Retransmit entry for tcb %u\n", (unsigned)(it-DPAUCS_transmissionControlBlocks) );

    uint16_t flags = it->cache.flags.ACK ? TCP_FLAG_ACK : 0;
    if( it->cache.flags.SYN )
      flags |= TCP_FLAG_SYN;

    if( it->cache.first && !it->cache.flags.SYN ){
      tcp_cache_entryInfo_t info;
      DPAUCS_tcp_cacheEntry_t*const last = DCE(*it->cache.last);
      DPAUCS_tcp_cacheEntry_t* next;
      DPAUCS_tcp_cacheEntry_t* e = DCE(*it->cache.first);
      do {
        uint16_t frag_flags = flags;
        next = 0;
        if( last == e && it->cache.flags.FIN && !it->cache.flags.SYN )
          flags |= TCP_FLAG_FIN;
        getEntryInfo( &info, e );
        DPA_stream_raw_t raw_stream = {
          .bufferBufferSize = e->bufferBufferSize,
          .charBufferSize   = e->charBufferSize,
          .bufferBuffer     = info.bufferBuffer,
          .charBuffer       = info.charBuffer
        };

        for( unsigned i = e->count; i--; ){
          if( e->ctcb[i].tcb == it ){
            frag_flags |= e->ctcb[i].flags;
            if( e->ctcb[i].next && *e->ctcb[i].next )
              next = DCE(*e->ctcb[i].next);
          }
        }

        DPAUCS_raw_as_stream(&raw_stream,&tcp_retransmission_cache_do_retransmissions_sub,(struct tcp_retransmission_cache_do_retransmissions_sub_args[]){{
          .tcb = it,
          .flags = flags,
          .size = e->streamRealLength,
          .SEQ = it->cache.first_SEQ
        }});

      } while(( e = next ));

    }else{
      if( it->cache.flags.FIN && !it->cache.flags.SYN )
        flags |= TCP_FLAG_FIN;
      DPAUCS_tcp_transmit( 0, it, flags, 0, it->cache.first_SEQ );
    }

    it->cache.flags.ACK = false;
  }
}
