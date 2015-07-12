#include "protocol/tcp_retransmission_cache.h"

cacheEntry_t* addToCache( DPAUCS_tcp_transmission_t* t, transmissionControlBlock_t** tcb, uint16_t flags ){
  (void)t;
  (void)tcb;
  (void)flags;
  return 0;
}

void accessCache( cacheEntry_t* entry, cacheAccessFunc_t func ){
  (void)entry;
  (void)func;
}

void removeFromCache( cacheEntry_t* entry ){
  (void)entry;
}
