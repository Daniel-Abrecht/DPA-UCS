#ifndef TCP_RETRANSMISSION_CACHE_H
#define TCP_RETRANSMISSION_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "protocol/tcp.h"

typedef void(*cacheAccessFunc_t)(DPAUCS_tcp_transmission_t* t, transmissionControlBlock_t** tcb, uint16_t flags);

typedef struct {
  size_t tcbBufferSize;
  size_t charBufferSize;
  size_t streamBufferSize;
} cacheEntry_t;

cacheEntry_t* addToCache( DPAUCS_tcp_transmission_t*, transmissionControlBlock_t**, uint16_t );
void accessCache( cacheEntry_t*, cacheAccessFunc_t );
void removeFromCache( cacheEntry_t* );

#endif
