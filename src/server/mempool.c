#include <stdbool.h>
#include <string.h>
#include <DPA/UCS/mempool.h>

static inline void* getEntryEnd(DPA_mempoolEntry_t* entry){
  return ((uint8_t*)entry) + DPAUCS_MEMPOOL_ENTRY_SIZE + entry->size;
}

static inline void* getEntryDatas(DPA_mempoolEntry_t* entry){
  return ((uint8_t*)entry) + DPAUCS_MEMPOOL_ENTRY_SIZE;
}

bool DPA_mempool_alloc( DPA_mempool_t*const mempool, void**const ptr, size_t size ){
  if(!size){
    *ptr = 0;
    return true;
  }

  // Add padding to ensure proper alignement of DPA_mempoolEntry elements
  size += DPAUCS_CALC_PADDING(size);

  const size_t entrySize = size + DPAUCS_MEMPOOL_ENTRY_SIZE;

  if( mempool->freeMemory < entrySize )
    return false;
  if( mempool->largestContiguousFreeMemorySize < entrySize )
    DPA_mempool_defragment(mempool);

  size_t areaSize = mempool->largestContiguousFreeMemorySize;
  void* secondLargestContiguousFreeMemoryBegin = 0;
  size_t secondLargestContiguousFreeMemorySize = 0;
  void* result = 0;
  DPA_mempoolEntry_t* lastEntry = 0;

  for(
    DPA_mempoolEntry_t* iterator = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
    iterator;
    iterator = iterator->nextEntry
  ){
    void* freeSpaceBegin = getEntryEnd(iterator);
    void* freeSpaceEnd = iterator->nextEntry;
    if(!freeSpaceEnd)
      freeSpaceEnd = (uint8_t*)mempool->firstEntry + mempool->size;
    size_t freeSpaceSize = (uintptr_t)freeSpaceEnd - (uintptr_t)freeSpaceBegin;
    if( freeSpaceBegin != mempool->largestContiguousFreeMemoryBegin
     && secondLargestContiguousFreeMemorySize < freeSpaceSize
    ){
      secondLargestContiguousFreeMemoryBegin = freeSpaceBegin;
      secondLargestContiguousFreeMemorySize  = freeSpaceSize;
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

  DPA_mempoolEntry_t* newEntry = result;

  newEntry->size = size;
  newEntry->reference = ptr;

  newEntry->nextEntry = lastEntry->nextEntry;
  newEntry->lastEntry = lastEntry;
  if(lastEntry)
    lastEntry->nextEntry = newEntry;
  if(newEntry->nextEntry)
    newEntry->nextEntry->lastEntry = newEntry;

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

bool DPA_mempool_realloc( DPA_mempool_t*const mempool, void**const memory, size_t size, bool preserveContentEnd ){

  if(!size){
    DPA_mempool_free( mempool, memory );
    return true;
  }

  DPA_mempoolEntry_t* entry = DPAUCS_GET_MEMPOOL_ENTRY( *memory );
  size += DPAUCS_CALC_PADDING(size);

  if( entry->reference != memory ) // invalid entry
    return false;

  long long diff = size - entry->size;

  if( !diff )
    return true;

  DPA_mempoolEntry_t* previous = entry->lastEntry;
  if( !previous )
    return false;

  if( (long long)mempool->freeMemory < diff )
    return false;

  mempool->freeMemory -= diff;

  if(preserveContentEnd){ // begin

    void* freeSpaceBegin = getEntryEnd(previous);
    size_t freeSpaceSize = (uintptr_t)entry - (uintptr_t)freeSpaceBegin;
    if( (long long)freeSpaceSize < diff )
      goto defragmentMoveResize;
    if( mempool->largestContiguousFreeMemoryBegin == freeSpaceBegin )
      mempool->largestContiguousFreeMemorySize += diff;
    DPA_mempoolEntry_t* newEntry = (void*)((char*)entry - diff);
    memmove( newEntry, entry, sizeof(*entry) );
    previous->nextEntry = entry = newEntry;
    if( entry->nextEntry )
      entry->nextEntry->lastEntry = entry;
    *entry->reference = getEntryDatas( entry );
    entry->size += diff;

  }else{ // end

    void* freeSpaceBegin = getEntryEnd(entry);
    size_t freeSpaceSize = (uintptr_t)entry->nextEntry - (uintptr_t)freeSpaceBegin;
    if( (long long)freeSpaceSize < diff )
      goto defragmentMoveResize;
    if( mempool->largestContiguousFreeMemoryBegin == freeSpaceBegin ){
      mempool->largestContiguousFreeMemorySize -= diff;
      entry->size += diff;
      mempool->largestContiguousFreeMemoryBegin = getEntryEnd(entry);
    }else{
      entry->size += diff;
    }

  }

  return true;

  defragmentMoveResize: {

    DPA_mempoolEntry_t* iterator;
    for(
      iterator = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
      iterator->nextEntry != entry;
      iterator = iterator->nextEntry
    ){
      if( iterator->nextEntry == getEntryEnd(iterator) )
        continue;
      memmove( getEntryEnd(iterator), iterator->nextEntry, iterator->nextEntry->size + DPAUCS_MEMPOOL_ENTRY_SIZE );
      iterator->nextEntry = getEntryEnd(iterator);
      iterator->nextEntry->lastEntry = iterator;
      *iterator->nextEntry->reference = getEntryDatas(iterator->nextEntry);
    }

    char* srcStart = getEntryDatas( entry );
    size_t size = entry->size;

    DPA_mempoolEntry_t* newEntry = getEntryEnd( iterator );
    memmove( newEntry, entry, sizeof(*entry) );
    iterator->nextEntry = entry = newEntry;
    entry->lastEntry = iterator;
    *entry->reference = getEntryDatas( entry );

    entry->size += diff;

    char* dstEnd = getEntryEnd( entry );
    char* dstStart = preserveContentEnd ? dstEnd - entry->size : getEntryDatas( entry );

    if( dstEnd > (char*)entry->nextEntry ){ // move following overlapping memory away

      size_t reqSize = (uintptr_t)dstEnd - (uintptr_t)srcStart + size;
      size_t s = 0;
      for(
        iterator = entry;
        iterator->nextEntry;
        iterator = iterator->nextEntry
      ){
        void* freeSpaceEnd = iterator->nextEntry;
        if(!freeSpaceEnd)
          freeSpaceEnd = (uint8_t*)mempool->firstEntry + mempool->size;
        s += (uintptr_t)getEntryEnd(iterator) - (uintptr_t)freeSpaceEnd;
        if( s >= reqSize )
          break;
      }

      DPA_mempoolEntry_t* tmp = iterator;
      size_t move = s - reqSize;
      for(;
        iterator->lastEntry != entry;
        iterator = iterator->lastEntry
      ){
        void* freeSpaceEnd = iterator->nextEntry;
        if(!freeSpaceEnd)
          freeSpaceEnd = (uint8_t*)mempool->firstEntry + mempool->size;
        void* dst = (char*)iterator->nextEntry - move - iterator->size - DPAUCS_MEMPOOL_ENTRY_SIZE;
        move = 0;
        memmove( dst, iterator, iterator->nextEntry->size + DPAUCS_MEMPOOL_ENTRY_SIZE );
        iterator = dst;
        iterator->lastEntry->nextEntry = iterator;
        if( iterator->nextEntry )
          iterator->nextEntry->lastEntry = iterator;
        *iterator->reference = getEntryDatas( iterator );
      }
      iterator = tmp;
    }

    memmove( dstStart, srcStart, size );

    for(
      iterator = entry; // dummyentry with mempool->firstEntry->size=0
      iterator->nextEntry != entry;
      iterator = iterator->nextEntry
    ){
      if( iterator->nextEntry == getEntryEnd(iterator) )
        continue;
      memmove( getEntryEnd(iterator), iterator->nextEntry, iterator->nextEntry->size + DPAUCS_MEMPOOL_ENTRY_SIZE );
      iterator->nextEntry = getEntryEnd(iterator);
      iterator->nextEntry->lastEntry = iterator;
      *iterator->nextEntry->reference = getEntryDatas(iterator->nextEntry);
    }

    mempool->largestContiguousFreeMemoryBegin = getEntryEnd(iterator);
    mempool->largestContiguousFreeMemorySize = (uintptr_t)mempool->firstEntry + (uintptr_t)mempool->size - (uintptr_t)mempool->largestContiguousFreeMemoryBegin;

  }

  return true;
}

void DPA_mempool_defragment( DPA_mempool_t*const mempool ){
  DPA_mempoolEntry_t* iterator;
  for(
    iterator = mempool->firstEntry; // dummyentry with mempool->firstEntry->size=0
    iterator->nextEntry;
    iterator = iterator->nextEntry
  ){
    if( iterator->nextEntry == getEntryEnd(iterator) )
      continue;
    memmove( getEntryEnd(iterator), iterator->nextEntry, iterator->nextEntry->size + DPAUCS_MEMPOOL_ENTRY_SIZE );
    iterator->nextEntry = getEntryEnd(iterator);
    iterator->nextEntry->lastEntry = iterator;
    *iterator->nextEntry->reference = getEntryDatas(iterator->nextEntry);
  }
  mempool->largestContiguousFreeMemoryBegin = getEntryEnd(iterator);
  mempool->largestContiguousFreeMemorySize = (uintptr_t)mempool->firstEntry + (uintptr_t)mempool->size - (uintptr_t)mempool->largestContiguousFreeMemoryBegin;
}

bool DPA_mempool_free( DPA_mempool_t*const mempool, void**const memory ){
  DPA_mempoolEntry_t* entry = DPAUCS_GET_MEMPOOL_ENTRY( *memory );

  DPA_mempoolEntry_t* previous = entry->lastEntry;
  if( !previous || entry->reference != memory ) // invalid entry
    return false;

  const size_t entrySize = entry->size + DPAUCS_MEMPOOL_ENTRY_SIZE;

  *memory = 0;
  previous->nextEntry = entry->nextEntry;
  if(previous->nextEntry)
    previous->nextEntry->lastEntry = previous;

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

void DPA_mempool_each( DPA_mempool_t*const mempool, bool(*handler)(void**,void*), void* arg ){
  DPA_mempoolEntry_t* iterator = mempool->firstEntry->nextEntry;
  if(!iterator)
    return;
  while(iterator){
    void** reference = iterator->nextEntry ? iterator->nextEntry->reference : 0;
    (*handler)(iterator->reference,arg);
    if(!reference)
      break;
    iterator = (DPA_mempoolEntry_t*)((uint8_t*)(*reference) - DPAUCS_MEMPOOL_ENTRY_SIZE);
  };
}
