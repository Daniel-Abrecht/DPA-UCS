#include <stdio.h>
#include <criterion/criterion.h>
#include <DPA/utils/mempool.h>

#define MTest(...) Test(__VA_ARGS__, .init = setup)

// TEST FOR: utils/mempool

static char buffer[1024+DPAUCS_MEMPOOL_ENTRY_SIZE];
static DPA_mempool_t mempool = {
  .size = sizeof(buffer)
};

void setup( void ){
  static DPA_mempool_t tmp = DPAUCS_MEMPOOL(buffer,sizeof(buffer));
  memset( buffer, 0, sizeof(buffer) );
  mempool.memory = tmp.memory;
  mempool.freeMemory = tmp.freeMemory;
  mempool.largestContiguousFreeMemorySize = tmp.largestContiguousFreeMemorySize;
  mempool.largestContiguousFreeMemoryBegin = tmp.largestContiguousFreeMemoryBegin;
}

MTest(mempool,alloc){
  int rems = sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 5;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char* ptr[5] = {"a","b","c","d","e"};
  char* orig[5];
  memcpy(orig,ptr,5*sizeof(char*));
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+0, rems / 4 ), "Allocation 1 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+1, rems / 4 ), "Allocation 2 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+2, rems / 4 ), "Allocation 3 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+3, rems / 4 ), "Allocation 4 failed" );
  cr_assert( !DPA_mempool_alloc( &mempool, (void**)ptr+4, rems / 4 ), "Allocation 5 didn't fail" );
}
