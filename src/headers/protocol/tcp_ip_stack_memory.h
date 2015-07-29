#ifndef TCP_IP_STACK_MEMORY_H
#define TCP_IP_STACK_MEMORY_H

#include <mempool.h>

#define STACK_BUFFER_SIZE 1024u * 5u // 50 KB
#define DPAUCS_MAX_FRAGMANTS 256

#define ANY_FRAGMENT ~0u

// Don't add more than 16 fragment types
// sizeof(enum) is equal to sizeof(int)
// int is 16 bit large on some platforms
enum DPAUCS_fragmentType {
#ifdef USE_IPv4
  DPAUCS_FRAGMENT_TYPE_IPv4 = 1 << 0,
#endif
#ifdef USE_IPv6
  DPAUCS_FRAGMENT_TYPE_IPv6 = 1 << 1,
#endif
};

typedef struct DPAUCS_fragment {
  // packetNumber: enumerate packets, drop oldest if ip stack is full
  // to find out the oldest packet, use
  // packetNumber-packetNumberCounter instantof
  // packetNumber to take care about overflows
  unsigned short packetNumber;
  uint16_t size; // payloadsize
  enum DPAUCS_fragmentType type;
} DPAUCS_fragment_t;

typedef struct {
  void(*destructor)(struct DPAUCS_fragment**);
  void(*beforeTakeover)(struct DPAUCS_fragment**);
} DPAUCS_fragment_info_t;

DPAUCS_fragment_t** DPAUCS_createFragment( enum DPAUCS_fragmentType type, size_t size );
void DPAUCS_removeOldestFragment( void );
void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment );
void DPAUCS_eachFragment( enum DPAUCS_fragmentType filter, bool(*handler)(DPAUCS_fragment_t**,void*), void* arg );
unsigned DPAUCS_getFragmentTypeSize(enum DPAUCS_fragmentType type);
void DPAUCS_takeover( DPAUCS_fragment_t** old, DPAUCS_fragment_t** new, enum DPAUCS_fragmentType type );
void* DPAUCS_getFragmentData( DPAUCS_fragment_t* fragment );

#endif
