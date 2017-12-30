#ifndef DPAUCS_FRAGMENT_MEMORY_H
#define DPAUCS_FRAGMENT_MEMORY_H

#ifndef STACK_BUFFER_SIZE
#define STACK_BUFFER_SIZE 1024u * 3u // 3 KiB
#endif

#ifndef DPA_MAX_FRAGMANTS
#define DPA_MAX_FRAGMANTS 32
#endif

struct DPAUCS_fragmentHandler;

typedef struct DPAUCS_fragment {
  // packetNumber: enumerate packets, drop oldest if ip stack is full
  // to find out the oldest packet, use
  // packetNumber-packetNumberCounter instantof
  // packetNumber to take care about overflows
  unsigned short packetNumber;
  uint16_t size; // payloadsize
  const flash struct DPAUCS_fragmentHandler* handler;
} DPAUCS_fragment_t;

DPAUCS_fragment_t** DPAUCS_createFragment( const flash struct DPAUCS_fragmentHandler* type, size_t size );
void DPAUCS_removeOldestFragment( void );
void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment );
void DPAUCS_eachFragment( const flash struct DPAUCS_fragmentHandler* filter, bool(*handler)(DPAUCS_fragment_t**,void*), void* arg );
unsigned DPAUCS_getFragmentTypeSize( const flash struct DPAUCS_fragmentHandler* type );
bool DPAUCS_takeover( DPAUCS_fragment_t** fragment, const flash struct DPAUCS_fragmentHandler* newType );
void* DPAUCS_getFragmentData( DPAUCS_fragment_t* fragment );

#endif
