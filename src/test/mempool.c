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
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+0, rems / 4 ), "Allocation 0 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+1, rems / 4 ), "Allocation 1 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+2, rems / 4 ), "Allocation 2 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+3, rems / 4 ), "Allocation 3 failed" );
  cr_assert( !DPA_mempool_alloc( &mempool, (void**)ptr+4, rems / 4 ), "Allocation 4 didn't fail" );
  cr_assert_neq( ptr[0], orig[0], "ptr[0] hasn't changed" );
  cr_assert_neq( ptr[1], orig[1], "ptr[1] hasn't changed" );
  cr_assert_neq( ptr[2], orig[2], "ptr[2] hasn't changed" );
  cr_assert_neq( ptr[3], orig[3], "ptr[3] hasn't changed" );
  cr_assert_eq( ptr[4], orig[4], "ptr[4] has changed" );
  cr_assert( ptr[0], "ptr[0] was null" );
  cr_assert( ptr[1], "ptr[1] was null" );
  cr_assert( ptr[2], "ptr[2] was null" );
  cr_assert( ptr[3], "ptr[3] was null" );
}


MTest(mempool,alloc_defragment){
  int rems = sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 5;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char* ptr[5] = {"a","b","c","d","e"};
  char *orig[5], *tmp[2];
  memcpy(orig,ptr,5*sizeof(char*));
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+0, rems / 4 ), "Allocation 0 failed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+1, rems / 4 ), "Allocation 1 failed" );
  cr_assert_neq( ptr[1], orig[1], "ptr[1] hasn't changed" );
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+2, rems / 4 ), "Allocation 2 failed" );
  cr_assert(  DPA_mempool_free( &mempool, (void**)ptr+1 ), "Couldn't free memory" );
  tmp[0] = ptr[0];
  tmp[1] = ptr[2];
  cr_assert(  DPA_mempool_alloc( &mempool, (void**)ptr+3, rems / 4 * 2 ), "Allocation 3 failed" );
  cr_assert( !DPA_mempool_alloc( &mempool, (void**)ptr+4, rems / 4 ), "Allocation 4 didn't fail" );
  cr_assert_neq( ptr[0], orig[0], "ptr[0] hasn't changed" );
  cr_assert_neq( ptr[2], orig[2], "ptr[2] hasn't changed" );
  cr_assert_neq( ptr[3], orig[3], "ptr[3] hasn't changed" );
  cr_assert_eq( ptr[0], tmp[0], "ptr[0] was moved" );
  cr_assert_neq( ptr[2], tmp[1], "ptr[1] wasn't moved" );
  cr_assert_eq( ptr[4], orig[4], "ptr[4] has changed" );
  cr_assert( ptr[0], "ptr[0] was null" );
  cr_assert( !ptr[1], "ptr[1] wasn't null" );
  cr_assert( ptr[2], "ptr[2] was null" );
  cr_assert( ptr[3], "ptr[3] was null" );
}
