#define TCP_IP_STACK_MEMORY_C
#include <protocol/tcp_ip_stack_memory.h>

static char buffer[STACK_BUFFER_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));

bool DPAUCS_enqueueFragment( enum DPAUCS_fragmentType type, DPAUCS_fragment_t* fragment, size_t size ){
  (void)mempool;
  (void)type;
  (void)fragment;
  (void)size;
  return false;
}

void DPAUCS_removeOutdatedFragments( void ){
  
}

void DPAUCS_removeOldestFragmentsUntilFreeMemoryIsAtLeast( size_t requiredFreeMemory ){
  (void)requiredFreeMemory;
}

void DPAUCS_removeFragment( DPAUCS_fragment_t* fragment ){
  (void)fragment;  
}

void DPAUCS_eachFragment( enum DPAUCS_fragmentType filter, bool(*handler)(DPAUCS_fragment_t*) ){
  (void)filter;
  (void)handler;
}
