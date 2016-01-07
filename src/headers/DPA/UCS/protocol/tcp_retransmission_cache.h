#ifndef TCP_RETRANSMISSION_CACHE_H
#define TCP_RETRANSMISSION_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/protocol/tcp.h>

#ifndef TCP_RETRANSMISSION_CACHE_SIZE
#define TCP_RETRANSMISSION_CACHE_SIZE 1024 * 4 // 2K
#endif

#ifndef TCP_RETRANSMISSION_CACHE_MAX_ENTRIES
#define TCP_RETRANSMISSION_CACHE_MAX_ENTRIES 128
#endif

typedef void(*cacheAccessFunc_t)(DPAUCS_tcp_transmission_t*, DPAUCS_transmissionControlBlock_t**, uint16_t);

typedef struct DPAUCS_tcp_cacheEntry {
  size_t charBufferSize;
  size_t bufferBufferSize;
  size_t streamRealLength;
  struct DPAUCS_tcp_cacheEntry** next;
  unsigned count;
  bool streamIsLonger; // if streamRealLength can't represent the full length
} DPAUCS_tcp_cacheEntry_t;


bool tcp_addToCache( DPAUCS_tcp_transmission_t*, unsigned, DPAUCS_transmissionControlBlock_t**, uint16_t* );
void tcp_removeFromCache( DPAUCS_tcp_cacheEntry_t** );
void tcp_cleanupCache( void );
void tcp_cacheCleanupTCB( DPAUCS_transmissionControlBlock_t* );
void tcp_retransmission_cache_do_retransmissions( void );

#endif
