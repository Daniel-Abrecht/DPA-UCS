#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <helper_macros.h>

#if __STDC_VERSION__ >= 201112L // c11
#include <stdalign.h>
#endif

#ifndef UINTPTR_MAX
#define UINTPTR_MAX (~(size_t)0)
typedef size_t uintptr_t;
#endif

#ifdef __alignof_is_defined
#define MAX_ALIGN alignof(max_align_t)
#elif defined(KEIL)
struct maxAlignHelperStruct {
    long double a;
    void* g;
    long d;
    double b;
    float c;
    int e;
    short f;
};
#define MAX_ALIGN __alignof__(struct maxAlignHelperStruct)
#else
#error "Can't calculate MAX_ALIGN: alignof isn't defined"
#endif

#define DPAUCS_CALC_PADDING(S) ( ( S & (MAX_ALIGN-1) ) ? MAX_ALIGN - ( S & (MAX_ALIGN-1) ) : 0 )
// Add padding to ensure proper alignement of any type of datas after DPAUCS_mempoolEntry_t
#define DPAUCS_MEMPOOL_ENTRY_SIZE ( sizeof(DPAUCS_mempoolEntry_t) + DPAUCS_CALC_PADDING(sizeof(DPAUCS_mempoolEntry_t)) )


#define DPAUCS_MEMPOOL(BUFFER,SIZE) { \
  .freeMemory = SIZE - DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .size = SIZE, \
  .largestContiguousFreeMemorySize = SIZE - DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .largestContiguousFreeMemoryBegin = BUFFER + DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .memory = BUFFER \
}

#define DPAUCS_GET_MEMPOOL_ENTRY( memory ) \
  ((DPAUCS_mempoolEntry_t*)((uint8_t*)(memory) - DPAUCS_MEMPOOL_ENTRY_SIZE))

#define DPAUCS_MEMPOOL_SIZE( memory ) \
  DPAUCS_GET_MEMPOOL_ENTRY( memory )->size

struct DPAUCS_mempoolEntry;
typedef struct DPAUCS_mempoolEntry {
  void** reference;
  size_t size;
  struct DPAUCS_mempoolEntry* nextEntry;
} DPAUCS_mempoolEntry_t;

typedef struct DPAUCS_mempool {
  size_t freeMemory;
  const size_t size;
  size_t largestContiguousFreeMemorySize;
  void* largestContiguousFreeMemoryBegin;
  union {
    void* memory;
    DPAUCS_mempoolEntry_t* firstEntry;
  };
} DPAUCS_mempool_t;

bool DPAUCS_mempool_alloc( DPAUCS_mempool_t* mempool, void** ptr, size_t size );
void DPAUCS_mempool_defragment( DPAUCS_mempool_t* mempool );
bool DPAUCS_mempool_free( DPAUCS_mempool_t* mempool, void** memory );
void DPAUCS_mempool_each( DPAUCS_mempool_t*const mempool, bool(*handler)(void**,void*), void* arg );

#endif
