#ifndef DPA_MEMPOOL_H
#define DPA_MEMPOOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <DPA/utils/helper_macros.h>

#ifndef UINTPTR_MAX
#define UINTPTR_MAX (~(size_t)0)
typedef size_t uintptr_t;
#endif

#ifdef __alignof_is_defined
#define MAX_ALIGN alignof(max_align_t)
#else
struct maxAlignHelperStruct {
  long double a;
  void* g;
  long d;
  double b;
  float c;
  int e;
  short f;
};
#define MAX_ALIGN DPA_ALIGNOF(struct maxAlignHelperStruct)
#endif

#define DPAUCS_CALC_PADDING(S) ( ( S & (MAX_ALIGN-1) ) ? MAX_ALIGN - ( S & (MAX_ALIGN-1) ) : 0 )
// Add padding to ensure proper alignement of any type of datas after DPA_mempoolEntry_t
#define DPAUCS_MEMPOOL_ENTRY_SIZE ( sizeof(DPA_mempoolEntry_t) + DPAUCS_CALC_PADDING(sizeof(DPA_mempoolEntry_t)) )


#define DPAUCS_MEMPOOL(BUFFER,SIZE) { \
  .freeMemory = SIZE - DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .size = SIZE, \
  .largestContiguousFreeMemorySize = SIZE - DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .largestContiguousFreeMemoryBegin = BUFFER + DPAUCS_MEMPOOL_ENTRY_SIZE, \
  .memory = BUFFER \
}

#define DPAUCS_GET_MEMPOOL_ENTRY( memory ) \
  ((DPA_mempoolEntry_t*)((uint8_t*)(memory) - DPAUCS_MEMPOOL_ENTRY_SIZE))

#define DPAUCS_MEMPOOL_SIZE( memory ) \
  DPAUCS_GET_MEMPOOL_ENTRY( memory )->size

typedef struct DPA_mempoolEntry {
  void** reference;
  size_t size;
  struct DPA_mempoolEntry* lastEntry;
  struct DPA_mempoolEntry* nextEntry;
} DPA_mempoolEntry_t;

typedef struct DPA_mempool {
  size_t freeMemory;
  const size_t size;
  size_t largestContiguousFreeMemorySize;
  void* largestContiguousFreeMemoryBegin;
  union {
    void* memory;
    DPA_mempoolEntry_t* firstEntry;
  };
} DPA_mempool_t;

bool DPA_mempool_alloc( DPA_mempool_t* mempool, void** ptr, size_t size );
void DPA_mempool_defragment( DPA_mempool_t* mempool );
bool DPA_mempool_free( DPA_mempool_t* mempool, void** memory );
void DPA_mempool_each( DPA_mempool_t*const mempool, bool(*handler)(void**,void*), void* arg );
bool DPA_mempool_realloc( DPA_mempool_t*const mempool, void**const memory, size_t size, bool preserveContentEnd );

#endif
