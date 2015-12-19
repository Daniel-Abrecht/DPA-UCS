#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <DPA/UCS/mempool.h>
#include <DPA/UCS/protocol/tcp_retransmission_cache.h>

static char buffer[TCP_RETRANSMISSION_CACHE_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
static void* cacheEntries[TCP_RETRANSMISSION_CACHE_MAX_ENTRIES];

typedef struct {
  cacheEntry_t* entry;
  transmissionControlBlock_t** TCBs;
  tcp_segment_t* SEGs;
  unsigned char* charBuffer;
  bufferInfo_t* streamBuffer;
} entryInfo_t;

static inline void GCC_BUGFIX_50925 getEntryInfo( entryInfo_t* res, void** entry ){
  res->entry = *entry;
  res->TCBs = (void*)( res->entry + 1 );
  res->SEGs = (void*)( (char*)res->TCBs + res->entry->tcbBufferSize );
  res->charBuffer = (unsigned char*)res->SEGs + res->entry->segmentBufferSize;
  res->streamBuffer = (void*)( res->charBuffer + res->entry->charBufferSize );
}

cacheEntry_t** addToCache( DPAUCS_tcp_transmission_t* t, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG ){

  cacheEntry_t e = {
    .count = count,
    .tcbBufferSize = count * sizeof(*tcb),
    .segmentBufferSize = count * sizeof(*SEG),
    .charBufferSize = BUFFER_SIZE( t->stream->buffer ) * sizeof(char),
    .streamBufferSize = BUFFER_SIZE( t->stream->buffer_buffer ) * sizeof(bufferInfo_t),
  };

  size_t fullSize = sizeof(e)
                  + e.tcbBufferSize
                  + e.segmentBufferSize
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
  memcpy( emem, &e , sizeof(e)           ); emem += sizeof(e);
  memcpy( emem, tcb, e.tcbBufferSize     ); emem += e.tcbBufferSize;
  memcpy( emem, SEG, e.segmentBufferSize ); emem += e.segmentBufferSize;

  DPAUCS_stream_to_raw_buffer(
    t->stream,
    (void*)( emem + e.charBufferSize ),
    e.streamBufferSize,
    (void*)( emem ),
    e.charBufferSize
  );

  // emem += e.streamBufferSize + e.charBufferSize

  return (cacheEntry_t**)entry;
}

void removeFromCache( cacheEntry_t** entry ){
  DPAUCS_mempool_free( &mempool, (void**)entry );
}

void cleanupCache(){
  void** entrys = cacheEntries + TCP_RETRANSMISSION_CACHE_MAX_ENTRIES;
  while( --entrys >= cacheEntries ){
    if(!*entrys)
      continue;
    entryInfo_t info;
    getEntryInfo( &info, entrys );
    unsigned* count = &info.entry->count;
    uint32_t min_unacknowledged_octets = ~0;
    for( unsigned i=0,j=0,n=*count; j<n; i++,j++ ){
      uint32_t unacknowledged_octets = info.SEGs[i].SEQ + info.SEGs[i].LEN - info.TCBs[i]->SND.UNA;
      if( !unacknowledged_octets || unacknowledged_octets > info.TCBs[i]->SND.NXT ){
        --i;
        --*count;
        continue;
      }
      if( min_unacknowledged_octets > unacknowledged_octets )
        min_unacknowledged_octets = unacknowledged_octets;
      info.TCBs[i] = info.TCBs[j];
      info.SEGs[i] = info.SEGs[j];
    }
    if(!*count){ // no tcb's left, remove entry
      removeFromCache( (cacheEntry_t**)entrys );
    }else if( min_unacknowledged_octets ){
/* TODO: remove unused memory
      for( tcp_segment_t *SEG=SEGs, *end=SEGs+*count; SEG<end; SEG++ ){
        SEG->SEQ += max_unacknowledged_octets;
        SEG->LEN -= max_unacknowledged_octets;
      }
*/
    }
  }
}

static void retransmit( entryInfo_t* ei ){
  (void)ei;
  printf("retransmit\n");
}

// TODO: Make this dynamic, it's required by RFC 793 Page 41
#define RETRANSMISSION_INTERVAL 500

static bool do_retransmissions( void** entry, void* unused ){
  (void)unused;
  entryInfo_t info;
  getEntryInfo( &info, entry );
  if(!adelay_done( &info.entry->adelay, RETRANSMISSION_INTERVAL ))
    return true;
  retransmit( &info );
  return true;
}

void tcp_retransmission_cache_do_retransmissions( void ){
  DPAUCS_mempool_each( &mempool, do_retransmissions, 0 );
}
