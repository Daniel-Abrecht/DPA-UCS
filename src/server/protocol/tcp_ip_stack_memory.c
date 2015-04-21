#define TCP_IP_STACK_MEMORY_C
#include <protocol/tcp_ip_stack_memory.h>
#include <stdint.h>

static char buffer[STACK_BUFFER_SIZE] = {0};
static DPAUCS_mempool_t mempool = DPAUCS_MEMPOOL(buffer,sizeof(buffer));

static unsigned short packetNumberCounter = 0;

static DPAUCS_fragment_t* fragments[DPAUCS_MAX_FRAGMANTS] = {0};
static unsigned short fragmentsUsed = 0;

static bool removeFragmentByPacketNumber(DPAUCS_fragment_t** fragment, void* packetNumber){
  if( *(unsigned short*)packetNumber != (*fragment)->packetNumber )
    return true;
  DPAUCS_removeFragment(fragment);
  return false;
}

DPAUCS_fragment_t** DPAUCS_createFragment( enum DPAUCS_fragmentType type, size_t size ){
  unsigned short packetNumber = packetNumberCounter++;
  DPAUCS_eachFragment(ANY_FRAGMENT,&removeFragmentByPacketNumber,&packetNumber);
  if( fragmentsUsed < DPAUCS_MAX_FRAGMANTS )
    DPAUCS_removeOldestFragment();
  unsigned short i;
  for(i=DPAUCS_MAX_FRAGMANTS;i--;)
    if(!fragments[i])
      break;
  if(i>=DPAUCS_MAX_FRAGMANTS)
    return 0;
  DPAUCS_fragment_t** fragment_ptr = fragments + i;
  while( !DPAUCS_mempool_alloc(&mempool,(void**)fragment_ptr,size) && fragmentsUsed )
    DPAUCS_removeOldestFragment();
  DPAUCS_fragment_t* fragment = *fragment_ptr;
  if(!fragment)
    return 0;
  fragment->type = type;
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
  DPAUCS_eachFragment(ANY_FRAGMENT,&findOldestFragment,&args);
  if(!args.oldestFragment)
    return;
  DPAUCS_removeFragment(args.oldestFragment);
}

void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment ){
  DPAUCS_mempool_free(&mempool,(void**)fragment);
  fragmentsUsed--;
}

struct eachFragmentArgs {
  enum DPAUCS_fragmentType filter;
  bool(*handler)(DPAUCS_fragment_t**,void*);
  void* arg;
};

static bool eachFragment_helper(void** mem,void* arg){
  struct eachFragmentArgs* args = arg;
  DPAUCS_fragment_t** fragment_ptr = (DPAUCS_fragment_t**)mem;
  if( args->filter & (*fragment_ptr)->type )
    return (*args->handler)(fragment_ptr,args->arg);
  return true;
}

void DPAUCS_eachFragment( enum DPAUCS_fragmentType filter, bool(*handler)(DPAUCS_fragment_t**,void*), void* arg ){
  struct eachFragmentArgs args = {filter,handler,arg};
  DPAUCS_mempool_each( &mempool, &eachFragment_helper, &args );
}
