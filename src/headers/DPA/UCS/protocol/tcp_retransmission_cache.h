#ifndef TCP_RETRANSMISSION_CACHE_H
#define TCP_RETRANSMISSION_CACHE_H

#include <stddef.h>
#include <stdbool.h>

#ifndef TCP_RETRANSMISSION_CACHE_SIZE
#define TCP_RETRANSMISSION_CACHE_SIZE 1024 * 4 // 2K
#endif

#ifndef TCP_RETRANSMISSION_CACHE_MAX_ENTRIES
#define TCP_RETRANSMISSION_CACHE_MAX_ENTRIES 128
#endif

struct DPAUCS_tcp_transmission;
struct DPAUCS_transmissionControlBlock;

typedef void(*cacheAccessFunc_t)(struct DPAUCS_tcp_transmission*, struct DPAUCS_transmissionControlBlock**, uint16_t);

typedef struct DPAUCS_tcp_cacheEntry {
  size_t charBufferSize;
  size_t bufferBufferSize;
  size_t streamRealLength;
  unsigned count;
  bool streamIsLonger; // if streamRealLength can't represent the full length
} DPAUCS_tcp_cacheEntry_t;


bool tcp_addToCache( struct DPAUCS_tcp_transmission*, unsigned, struct DPAUCS_transmissionControlBlock**, uint16_t* );
void tcp_removeFromCache( DPAUCS_tcp_cacheEntry_t** );
void tcp_cleanupCache( void );
void tcp_cacheCleanupTCB( struct DPAUCS_transmissionControlBlock* );
void tcp_retransmission_cache_do_retransmissions( void );

#endif
