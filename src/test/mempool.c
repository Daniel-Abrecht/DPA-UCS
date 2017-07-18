#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
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


MTest(mempool,DPA_mempool_alloc){
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

#define reallocTestCase(X,Y) \
MTest(mempool,Y){ \
  int rems = sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 5; \
  cr_assert_gt(rems,0,"Not enougth buffer size"); \
  void* ptr[5] = {0}; \
  cr_assert(  DPA_mempool_realloc( &mempool, ptr+0, rems / 4, X ), "Allocation 0 failed" ); \
  cr_assert(  DPA_mempool_realloc( &mempool, ptr+1, rems / 4, X ), "Allocation 1 failed" ); \
  cr_assert(  DPA_mempool_realloc( &mempool, ptr+2, rems / 4, X ), "Allocation 2 failed" ); \
  cr_assert(  DPA_mempool_realloc( &mempool, ptr+3, rems / 4, X ), "Allocation 3 failed" ); \
  cr_assert( !DPA_mempool_realloc( &mempool, ptr+4, rems / 4, X ), "Allocation 4 didn't fail" ); \
  cr_assert( ptr[0], "ptr[0] was null" ); \
  cr_assert( ptr[1], "ptr[1] was null" ); \
  cr_assert( ptr[2], "ptr[2] was null" ); \
  cr_assert( ptr[3], "ptr[3] was null" ); \
  cr_assert( !ptr[4], "ptr[4] wasn't null" ); \
}

reallocTestCase( true, alloc_using_realloc_lp_true )
reallocTestCase( false, alloc_using_realloc_lp_false )

#undef reallocTestCase

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

MTest(mempool,free){
  void* ptr = 0;
  cr_assert( !DPA_mempool_free( &mempool, 0 ), "Free 0 succeded" );
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, 1 ), "Allocation failed" );
  cr_assert( ptr, "ptr was null" );
  cr_assert( DPA_mempool_free( &mempool, &ptr ), "Free 1 failed" );
  cr_assert( !ptr, "ptr was not null" );
  cr_assert( DPA_mempool_free( &mempool, &ptr ), "Free 2 failed" );
}

MTest(mempool,DPA_mempool_realloc_check_no_space_fail){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char memory[2][rems];
  for( char *it = *memory, *end = it+rems*2; it < end; it++ )
    *it = rand();
  void* ptr[2] = {0};
  void* orig[2];
  cr_assert( DPA_mempool_alloc( &mempool, ptr+0, rems ), "Allocation 0 failed" );
  cr_assert( DPA_mempool_alloc( &mempool, ptr+1, rems ), "Allocation 1 failed" );
  cr_assert( ptr[0], "ptr[0] was null" );
  cr_assert( ptr[1], "ptr[1] was null" );
  memcpy(orig,ptr,2*sizeof(void*));
  memcpy(ptr[0],memory[0],rems);
  memcpy(ptr[1],memory[1],rems);
  cr_assert( !DPA_mempool_realloc( &mempool, ptr+0, rems*2, false ), "Reallocation 0 didn't fail" );
  cr_assert_eq( ptr[0], orig[0], "ptr[0] has changed" );
  cr_assert_eq( ptr[1], orig[1], "ptr[1] has changed" );
  cr_assert( !memcmp(ptr[0],memory[0],rems), "Memory area 0 changed" );
  cr_assert( !memcmp(ptr[1],memory[1],rems), "Memory area 1 changed" );
}

MTest(mempool,DPA_mempool_realloc_check_grow_simple_preserve_end_defragment){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems*2, true ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_eq( ptr, orig, "ptr has changed" );
  cr_assert( !memcmp((char*)ptr+rems,memory,rems), "Memory changed" );
}

MTest(mempool,DPA_mempool_realloc_check_grow_simple_preserve_end_normal){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*tmp,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &tmp, rems ), "Allocation 0 failed" );
  cr_assert( tmp, "tmp is null" );
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 1 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert( DPA_mempool_free( &mempool, &tmp ), "Couldn't free memory" );
  cr_assert( !tmp, "tmp isn't null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems*2, true ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_neq( ptr, orig, "ptr hasn't changed" );
  cr_assert( !memcmp((char*)ptr+rems,memory,rems), "Memory changed" );
}

MTest(mempool,DPA_mempool_realloc_check_grow_simple_preserve_begin_defragment){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*tmp,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &tmp, rems ), "Allocation 0 failed" );
  cr_assert( tmp, "tmp is null" );
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 1 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert( DPA_mempool_free( &mempool, &tmp ), "Couldn't free memory" );
  cr_assert( !tmp, "tmp isn't null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems*2, false ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_eq( ptr, orig, "ptr has changed" );
  cr_assert( !memcmp(ptr,memory,rems), "Memory changed" );
}

MTest(mempool,DPA_mempool_realloc_check_grow_simple_preserve_begin){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 0 failed" );
  cr_assert( ptr, "ptr was null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems*2, false ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_eq( ptr, orig, "ptr has changed" );
  cr_assert( !memcmp(ptr,memory,rems), "Memory changed" );
}

MTest(mempool,DPA_mempool_realloc_check_shrink_simple_preserve_end){
  const int rems = sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3;
  cr_assert_gt(rems,1,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 0 failed" );
  cr_assert( ptr, "ptr was null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems/2, true ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_neq( ptr, orig, "ptr hasn't changed" );
  cr_assert( !memcmp(ptr,memory+rems/2,rems/2), "Memory changed" );
}


MTest(mempool,DPA_mempool_realloc_check_shrink_simple_preserve_begin){
  const int rems = sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3;
  cr_assert_gt(rems,1,"Not enougth buffer size");
  char memory[rems];
  for( char *it = memory, *end = it+rems; it < end; it++ )
    *it = rand();
  void *ptr,*orig;
  cr_assert( DPA_mempool_alloc( &mempool, &ptr, rems ), "Allocation 0 failed" );
  cr_assert( ptr, "ptr was null" );
  orig = ptr;
  memcpy(ptr,memory,rems);
  cr_assert( DPA_mempool_realloc( &mempool, &ptr, rems/2, false ), "Reallocation 0 failed" );
  cr_assert( ptr, "ptr is null" );
  cr_assert_eq( ptr, orig, "ptr has changed" );
  cr_assert( !memcmp(ptr,memory,rems/2), "Memory changed" );
}

struct testsub_DPA_mempool_each_params {
  void *ptr[2];
  int i;
};

bool testsub_0_DPA_mempool_each( void** entry, void* param ){
  (void)entry;
  (void)param;
  cr_assert( false, "Shouldn't have been called" );
  return false;
}

bool testsub_1_DPA_mempool_each( void** entry, void* param ){
  struct testsub_DPA_mempool_each_params* p = param;
  cr_assert( p->i < 2, "Only 2 entries available, but called for a 3rd entry" );
  cr_assert_eq( p->ptr + p->i, entry, "Wrong entry pointer" );
  p->i += 1;
  return true;
}

bool testsub_2_DPA_mempool_each( void** entry, void* param ){
  struct testsub_DPA_mempool_each_params* p = param;
  cr_assert( p->i < 1, "Entry called a second time, despite returning false previously" );
  cr_assert_eq( p->ptr, entry, "Wrong entry pointer" );
  p->i += 1;
  return false;
}

MTest(mempool,DPA_mempool_each){
  const int rems = ( sizeof(buffer) - DPAUCS_MEMPOOL_ENTRY_SIZE * 3 ) / 2;
  cr_assert_gt(rems,0,"Not enougth buffer size");
  struct testsub_DPA_mempool_each_params helper;
  helper.i = 0;
  DPA_mempool_each( &mempool, &testsub_0_DPA_mempool_each, &helper );
  cr_assert( DPA_mempool_alloc( &mempool, &helper.ptr[0], rems ), "Allocation 0 failed" );
  cr_assert( helper.ptr[0], "ptr[0] is null" );
  cr_assert( DPA_mempool_alloc( &mempool, &helper.ptr[1], rems ), "Allocation 1 failed" );
  cr_assert( helper.ptr[1], "ptr[1] is null" );
  helper.i = 0;
  DPA_mempool_each( &mempool, &testsub_1_DPA_mempool_each, &helper );
  helper.i = 0;
  DPA_mempool_each( &mempool, &testsub_1_DPA_mempool_each, &helper );
  helper.i = 0;
  DPA_mempool_each( &mempool, &testsub_2_DPA_mempool_each, &helper );
  helper.i = 0;
  DPA_mempool_each( &mempool, &testsub_1_DPA_mempool_each, &helper );
}
