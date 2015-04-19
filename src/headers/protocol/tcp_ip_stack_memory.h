#ifndef TCP_IP_STACK_MEMORY_H
#define TCP_IP_STACK_MEMORY_H

#include <mempool.h>

#define STACK_BUFFER_SIZE 1024 * 50 // 50 KB
#define ANY_FRAGMENT ~0u

// Don't add more than 16 fragment types
// sizeof(enum) is equal to sizeof(int)
// int is 16 bit large on some platforms
enum DPAUCS_fragmentType {
  FRAGMENT_TYPE_DPAUCS_ip_fragment_t = 1 << 0
};

typedef struct {
  // packetNumber: enumerate packets, drop oldest if ip stack is full
  // to find out the oldest packet, use
  // packetNumber-packetNumberCounter instantof
  // packetNumber to take care about overflows
  unsigned short packetNumber;
  uint16_t size; // payloadsize
  enum DPAUCS_fragmentType fragmentType;
} DPAUCS_fragment_t;

bool DPAUCS_enqueueFragment( enum DPAUCS_fragmentType type, DPAUCS_fragment_t*, size_t size );
void DPAUCS_removeOutdatedFragments( void );
void DPAUCS_removeOldestFragmentsUntilFreeMemoryIsAtLeast( size_t requiredFreeMemory );
void DPAUCS_removeFragment( DPAUCS_fragment_t* fragment );
void DPAUCS_eachFragment( enum DPAUCS_fragmentType filter, bool(*handler)(DPAUCS_fragment_t*) );

#ifndef TCP_IP_STACK_MEMORY_C
#define DPAUCS_getFragmentPayload(F) ((void*)(F+1))
#define DPAUCS_enqueueFragment(T,F) \
  DPAUCS_enqueueFragment(FRAGMENT_TYPE_ ## T,&F->fragment_info,sizeof(*F)+F->fragment_info.size)
#define DPAUCS_removeFragment(F) DPAUCS_removeFragment(&F->fragment_info)
#endif

#endif
