#ifndef DPAUCS_TCP_RETRANSMISSION_CACHE_H
#define DPAUCS_TCP_RETRANSMISSION_CACHE_H

#include <stddef.h>
#include <stdbool.h>

#ifndef TCP_RETRANSMISSION_CACHE_SIZE
#define TCP_RETRANSMISSION_CACHE_SIZE 1024 * 1 // 1K
#endif

#ifndef TCP_RETRANSMISSION_CACHE_MAX_ENTRIES
#define TCP_RETRANSMISSION_CACHE_MAX_ENTRIES 32
#endif

struct DPAUCS_tcp_transmission;
struct DPAUCS_transmissionControlBlock;
struct DPAUCS_tcp_cacheEntry;

typedef void(*cacheAccessFunc_t)(struct DPAUCS_tcp_transmission*, struct DPAUCS_transmissionControlBlock**, uint16_t);

bool tcp_addToCache( struct DPAUCS_tcp_transmission*, unsigned, struct DPAUCS_transmissionControlBlock**, uint16_t* );
void tcp_removeFromCache( struct DPAUCS_tcp_cacheEntry** );
void tcp_cleanupCache( void );
void tcp_cacheCleanupTCB( struct DPAUCS_transmissionControlBlock* );
void tcp_retransmission_cache_do_retransmissions( void );

#endif
