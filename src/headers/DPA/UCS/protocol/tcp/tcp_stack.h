#ifndef DPAUCS_TCP_STACK_H
#define DPAUCS_TCP_STACK_H

#include <DPA/UCS/protocol/fragment_memory.h>

typedef struct DPAUCS_tcp_fragment {
  DPAUCS_fragment_t fragment; // must be the first member
  struct DPAUCS_tcp_fragment** next;
} DPAUCS_tcp_fragment_t;

struct DPAUCS_transmissionControlBlock;

bool DPAUCS_tcp_cache_add( DPAUCS_fragment_t** fragment, struct DPAUCS_transmissionControlBlock* tcb );
void DPAUCS_tcp_cache_remove( struct DPAUCS_transmissionControlBlock* );

#endif
