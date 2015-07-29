#include <stdbool.h>
#include <string.h>
#include <mempool.h>

static inline void* getEntryEnd(DPAUCS_mempoolEntry_t* entry){
  return ((uint8_t*)entry) + DPAUCS_MEMPOOL_ENTRY_SIZE + entry->size;
}

static inline void* getEntryDatas(DPAUCS_mempoolEntry_t* entry){
  return ((uint8_t*)entry) + DPAUCS_MEMPOOL_ENTRY_SIZE;
}

bool DPAUCS_mempool_alloc( DPAUCS_mempool_t*const mempool, void**const ptr, size_t size ){
  if(!size){
    *ptr = 0;
    return true;
  }

  // Add padding to ensure proper alignement of DPAUCS_mempoolEntry elements
  size += DPAUCS_CALC_PADDING(size);

  const size_t entrySize = size + DPAUCS_MEMPOOL_ENTRY_SIZE;

  if( mempool->freeMemory < entrySize )
    return false;
  if( mempool->largestContiguousFreeMemorySize < entrySize )
    DPAUCS_mempool_defragment(mempool);

  size_t areaSize = mempool->largestContiguousFreeMemorySize;
  void* secondLargestContiguousFreeMemoryBegin = 0;
  size_t secondLargestContiguousFreeMemorySize = 0;
  void* result = 0;
  DPAUCS_mempoolEntry_t* lastEntry;

  for(
    DPAUCS_mempoolEntry_t* iterator = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
    iterator;
    iterator = iterator->nextEntry
  ){
    void* freeSpaceBegin = getEntryEnd(iterator);
    void* freeSpaceEnd = iterator->nextEntry;
    if(!freeSpaceEnd)
      freeSpaceEnd = (uint8_t*)mempool->firstEntry + mempool->size;
    size_t freeSpaceSize = (uintptr_t)freeSpaceEnd - (uintptr_t)freeSpaceBegin;
    if(
        freeSpaceBegin != mempool->largestContiguousFreeMemoryBegin
     && secondLargestContiguousFreeMemorySize < freeSpaceSize
    ){
      secondLargestContiguousFreeMemoryBegin = freeSpaceBegin;
      secondLargestContiguousFreeMemorySize = freeSpaceSize;
    }
    if( 
        freeSpaceSize < entrySize
     || areaSize < freeSpaceSize
    ) continue;
    areaSize = freeSpaceSize;
    result = freeSpaceBegin;
    lastEntry = iterator;
  }

  if(!result) // shouldn't happen
    return false;

  DPAUCS_mempoolEntry_t* newEntry = result;

  newEntry->size = size;
  newEntry->reference = ptr;

  newEntry->nextEntry = lastEntry->nextEntry;
  lastEntry->nextEntry = newEntry;

  if( mempool->largestContiguousFreeMemoryBegin == newEntry ){
    mempool->largestContiguousFreeMemorySize -= entrySize;
    mempool->largestContiguousFreeMemoryBegin = getEntryEnd(newEntry);
  }
  if( mempool->largestContiguousFreeMemorySize < secondLargestContiguousFreeMemorySize ){
    mempool->largestContiguousFreeMemorySize = secondLargestContiguousFreeMemorySize;
    mempool->largestContiguousFreeMemoryBegin = secondLargestContiguousFreeMemoryBegin;
  }

  mempool->freeMemory -= entrySize;

  *ptr = getEntryDatas(newEntry);

  return true;
}

void DPAUCS_mempool_defragment( DPAUCS_mempool_t*const mempool ){
  DPAUCS_mempoolEntry_t* iterator;
  for(
    iterator = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
    iterator && iterator->nextEntry;
    iterator = iterator->nextEntry
  ){
    if( iterator->nextEntry == getEntryEnd(iterator) )
      continue;
    memmove( getEntryEnd(iterator), iterator->nextEntry, iterator->nextEntry->size + DPAUCS_MEMPOOL_ENTRY_SIZE );
    iterator->nextEntry = getEntryEnd(iterator);
    *iterator->nextEntry->reference = getEntryDatas(iterator->nextEntry);
  }
  mempool->largestContiguousFreeMemoryBegin = getEntryEnd(iterator);
  mempool->largestContiguousFreeMemorySize = (uintptr_t)mempool->firstEntry + (uintptr_t)mempool->size - (uintptr_t)mempool->largestContiguousFreeMemoryBegin;
}

bool DPAUCS_mempool_free( DPAUCS_mempool_t*const mempool, void**const memory ){
  DPAUCS_mempoolEntry_t* entry = DPAUCS_GET_MEMPOOL_ENTRY( *memory );
  DPAUCS_mempoolEntry_t* previous;

  for(
    previous = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
    previous && previous->nextEntry != entry;
    previous = previous->nextEntry
  );

  if( !previous || entry->reference != memory ) // invalid entry
    return false;

  const size_t entrySize = entry->size + DPAUCS_MEMPOOL_ENTRY_SIZE;

  *memory = 0;
  previous->nextEntry = entry->nextEntry;

  void* freeSpaceBegin = getEntryEnd(previous);
  void* freeSpaceEnd = previous->nextEntry;
  if(!freeSpaceEnd)
    freeSpaceEnd = (uint8_t*)mempool->firstEntry + mempool->size;
  size_t freeSpaceSize = (uintptr_t)freeSpaceEnd - (uintptr_t)freeSpaceBegin;

  if( mempool->largestContiguousFreeMemorySize < freeSpaceSize ){
    mempool->largestContiguousFreeMemoryBegin = freeSpaceBegin;
    mempool->largestContiguousFreeMemorySize = freeSpaceSize;
  }

  mempool->freeMemory += entrySize;

  return true;
}

void DPAUCS_mempool_each( DPAUCS_mempool_t*const mempool, bool(*handler)(void**,void*), void* arg ){
  DPAUCS_mempoolEntry_t* iterator = mempool->firstEntry->nextEntry;
  if(!iterator)
    return;
  while(iterator){
    void** reference = iterator->nextEntry ? iterator->nextEntry->reference : 0;
    (*handler)(iterator->reference,arg);
    if(!reference)
      break;
    iterator = (DPAUCS_mempoolEntry_t*)((uint8_t*)(*reference) - DPAUCS_MEMPOOL_ENTRY_SIZE);
  };
}
