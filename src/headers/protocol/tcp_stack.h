#ifndef TCP_STACK_H
#define TCP_STACK_H

#include <protocol/tcp_ip_stack_memory.h>

typedef struct transmissionControlBlock transmissionControlBlock_t;
typedef struct DPAUCS_tcp_fragment DPAUCS_tcp_fragment_t;

typedef struct DPAUCS_tcp_fragment {
  DPAUCS_fragment_t fragment; // must be the first member
  DPAUCS_tcp_fragment_t** next;
} DPAUCS_tcp_fragment_t;

bool DPAUCS_tcp_cache_add( DPAUCS_fragment_t** fragment, transmissionControlBlock_t* tcb );
void DPAUCS_tcp_cache_remove( transmissionControlBlock_t* );

#endif
