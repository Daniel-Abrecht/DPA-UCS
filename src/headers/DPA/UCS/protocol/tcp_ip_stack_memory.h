#ifndef TCP_IP_STACK_MEMORY_H
#define TCP_IP_STACK_MEMORY_H

#include <DPA/UCS/mempool.h>

#define STACK_BUFFER_SIZE 1024u * 5u // 50 KB
#define DPAUCS_MAX_FRAGMANTS 256

// Don't add more than 16 fragment types
// sizeof(enum) is equal to sizeof(int)
// int is 16 bit large on some platforms
enum DPAUCS_fragmentType {
#ifdef USE_TCP
  DPAUCS_FRAGMENT_TYPE_TCP,
#endif
#ifdef USE_IPv4
  DPAUCS_FRAGMENT_TYPE_IPv4,
#endif
#ifdef USE_IPv6
  DPAUCS_FRAGMENT_TYPE_IPv6,
#endif
  DPAUCS_ANY_FRAGMENT
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
  void(*destructor)(DPAUCS_fragment_t**);
  bool(*beforeTakeover)(DPAUCS_fragment_t***,enum DPAUCS_fragmentType);
  void(*takeoverFailtureHandler)(DPAUCS_fragment_t**);
} DPAUCS_fragment_info_t;

DPAUCS_fragment_t** DPAUCS_createFragment( enum DPAUCS_fragmentType type, size_t size );
void DPAUCS_removeOldestFragment( void );
void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment );
void DPAUCS_eachFragment( enum DPAUCS_fragmentType filter, bool(*handler)(DPAUCS_fragment_t**,void*), void* arg );
unsigned DPAUCS_getFragmentTypeSize(enum DPAUCS_fragmentType type);
bool DPAUCS_takeover( DPAUCS_fragment_t** fragment, enum DPAUCS_fragmentType newType );
void* DPAUCS_getFragmentData( DPAUCS_fragment_t* fragment );

#endif
