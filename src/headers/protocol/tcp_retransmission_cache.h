#ifndef TCP_RETRANSMISSION_CACHE_H
#define TCP_RETRANSMISSION_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <adelay.h>
#include <protocol/tcp.h>

#ifndef TCP_RETRANSMISSION_CACHE_SIZE
#define TCP_RETRANSMISSION_CACHE_SIZE 1024 * 4 // 2K
#endif

#ifndef TCP_RETRANSMISSION_CACHE_MAX_ENTRIES
#define TCP_RETRANSMISSION_CACHE_MAX_ENTRIES 128
#endif

typedef void(*cacheAccessFunc_t)(DPAUCS_tcp_transmission_t* t, transmissionControlBlock_t** tcb, uint16_t flags);

typedef struct {
  unsigned count;
  size_t tcbBufferSize;
  size_t segmentBufferSize;
  size_t charBufferSize;
  size_t streamBufferSize;
  adelay_t adelay;
} cacheEntry_t;


cacheEntry_t** addToCache( DPAUCS_tcp_transmission_t*, unsigned, transmissionControlBlock_t**, tcp_segment_t* );
void removeFromCache( cacheEntry_t** );
void cleanupCache();
void tcp_retransmission_cache_do_retransmissions( void );

#endif
