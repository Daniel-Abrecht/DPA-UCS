#include <stdint.h>
#include <DPA/utils/mempool.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/ip_stack.h>
#include <DPA/UCS/protocol/tcp_stack.h>
#include <DPA/UCS/protocol/tcp_ip_stack_memory.h>

static char buffer[STACK_BUFFER_SIZE] = {0};
static DPA_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));

static unsigned short packetNumberCounter = 0;

static DPAUCS_fragment_t* fragments[DPA_MAX_FRAGMANTS] = {0};
static unsigned short fragmentsUsed = 0;

static bool removeFragmentByPacketNumber(DPAUCS_fragment_t** fragment, void* packetNumber){
  if( *(unsigned short*)packetNumber != (*fragment)->packetNumber )
    return true;
  DPAUCS_removeFragment(fragment);
  return false;
}

bool DPAUCS_takeover( DPAUCS_fragment_t** fragment, DPAUCS_fragmentHandler_t* newHandler ){
  DPAUCS_fragment_t tmp = **fragment;
  bool(*beforeTakeover)(DPAUCS_fragment_t***,DPAUCS_fragmentHandler_t*) = (*fragment)->handler->beforeTakeover;
  if( beforeTakeover )
    if(! (*beforeTakeover)( &fragment, newHandler ) )
      return false;
  if( fragment < fragments || fragment > fragments + DPA_MAX_FRAGMANTS
   || !DPA_mempool_realloc( &mempool, (void**)fragment, tmp.size + newHandler->fragmentInfo_size, true )
  ){
    void(*takeoverFailtureHandler)(DPAUCS_fragment_t**) = (*fragment)->handler->takeoverFailtureHandler;
    if(takeoverFailtureHandler)
      (*takeoverFailtureHandler)( fragment );
    return false;
  }
  **fragment = tmp;
  (*fragment)->packetNumber = packetNumberCounter++;
  (*fragment)->handler = newHandler;
  return true;
}

void* DPAUCS_getFragmentData( DPAUCS_fragment_t* fragment ){
  return (char*)fragment + DPAUCS_MEMPOOL_SIZE( fragment ) - fragment->size;
}

DPAUCS_fragment_t** DPAUCS_createFragment( DPAUCS_fragmentHandler_t* handler, size_t size ){
  size_t fragmentTypeSize = handler->fragmentInfo_size;
  unsigned short packetNumber = packetNumberCounter++;
  DPAUCS_eachFragment(0,&removeFragmentByPacketNumber,&packetNumber);
  if( fragmentsUsed < DPA_MAX_FRAGMANTS )
    DPAUCS_removeOldestFragment();
  unsigned short i;
  for(i=DPA_MAX_FRAGMANTS;i--;)
    if(!fragments[i])
      break;
  if(i>=DPA_MAX_FRAGMANTS)
    return 0;
  DPAUCS_fragment_t** fragment_ptr = fragments + i;
  while( !DPA_mempool_alloc(&mempool,(void**)fragment_ptr,size + fragmentTypeSize) && fragmentsUsed )
    DPAUCS_removeOldestFragment();
  DPAUCS_fragment_t* fragment = *fragment_ptr;
  if(!fragment)
    return 0;
  fragment->handler = handler;
  fragment->size = size;
  fragment->packetNumber = packetNumber;
  fragmentsUsed++;
  return fragment_ptr;
}

struct findOldestFragmentArgs {
  unsigned short oldestTime;
  DPAUCS_fragment_t** oldestFragment;
};

static bool findOldestFragment( DPAUCS_fragment_t** fragment_ptr, void* _args ){
  struct findOldestFragmentArgs* args = _args;
  DPAUCS_fragment_t* fragment = *fragment_ptr;
  unsigned short time = fragment->packetNumber - packetNumberCounter;
  if( time > args->oldestTime )
    return true;
  args->oldestTime = time;
  args->oldestFragment = fragment_ptr;
  return true;
}

void DPAUCS_removeOldestFragment( void ){
  struct findOldestFragmentArgs args = {(unsigned short)~0u,0};
  DPAUCS_eachFragment(0,&findOldestFragment,&args);
  if(!args.oldestFragment)
    return;
  DPAUCS_removeFragment(args.oldestFragment);
}

void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment ){
  void(*destructor)(DPAUCS_fragment_t**) = (*fragment)->handler->destructor;
  if(destructor)
    (*destructor)(fragment);
  DPA_mempool_free(&mempool,(void**)fragment);
  fragmentsUsed--;
}

struct eachFragmentArgs {
  DPAUCS_fragmentHandler_t* handler;
  bool(*callback)(DPAUCS_fragment_t**,void*);
  void* arg;
};

static bool eachFragment_helper(void** mem,void* arg){
  struct eachFragmentArgs* args = arg;
  DPAUCS_fragment_t** fragment_ptr = (DPAUCS_fragment_t**)mem;
  if( args->handler || args->handler == (*fragment_ptr)->handler )
    return (*args->callback)(fragment_ptr,args->arg);
  return true;
}

void DPAUCS_eachFragment( DPAUCS_fragmentHandler_t* handler, bool(*callback)(DPAUCS_fragment_t**,void*), void* arg ){
  struct eachFragmentArgs args = {handler,callback,arg};
  DPA_mempool_each( &mempool, &eachFragment_helper, &args );
}
