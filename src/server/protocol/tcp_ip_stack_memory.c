#include <stdint.h>
#include <DPA/UCS/protocol/tcp_ip_stack_memory.h>
#include <DPA/UCS/protocol/ip_stack.h>
#include <DPA/UCS/protocol/tcp_stack.h>


extern const DPAUCS_fragment_info_t DPAUCS_ip_fragment_info;
extern const DPAUCS_fragment_info_t DPAUCS_tcp_fragment_info;

static const DPAUCS_fragment_info_t*const fragmentTypeInfos[] = {
#ifdef USE_TCP
  &DPAUCS_tcp_fragment_info,
#endif
#ifdef USE_IPv4
  &DPAUCS_ip_fragment_info,
#endif
#ifdef USE_IPv6
  &DPAUCS_ip_fragment_info,
#endif
  0
};


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

unsigned DPAUCS_getFragmentTypeSize(enum DPAUCS_fragmentType type){
  switch(type){
#ifdef USE_TCP
    case DPAUCS_FRAGMENT_TYPE_TCP: return sizeof(DPAUCS_tcp_fragment_t);
#endif
#ifdef USE_IPv4
    case DPAUCS_FRAGMENT_TYPE_IPv4: return sizeof(DPAUCS_IPv4_fragment_t);
#endif
    case DPAUCS_ANY_FRAGMENT: break;
  }
  return 0;
}

bool DPAUCS_takeover( DPAUCS_fragment_t** fragment, enum DPAUCS_fragmentType newType ){
  DPAUCS_fragment_t tmp = **fragment;
  bool(*beforeTakeover)(DPAUCS_fragment_t***,enum DPAUCS_fragmentType) = fragmentTypeInfos[(*fragment)->type]->beforeTakeover;
  if( beforeTakeover )
    if(! (*beforeTakeover)( &fragment, newType ) )
      return false;
  if( fragment < fragments || fragment > fragments + DPAUCS_MAX_FRAGMANTS
   || !DPAUCS_mempool_realloc( &mempool, (void**)fragment, tmp.size + DPAUCS_getFragmentTypeSize( newType ), true )
  ){
    void(*takeoverFailtureHandler)(DPAUCS_fragment_t**) = fragmentTypeInfos[(*fragment)->type]->takeoverFailtureHandler;
    if(takeoverFailtureHandler)
      (*takeoverFailtureHandler)( fragment );
    return false;
  }
  **fragment = tmp;
  (*fragment)->packetNumber = packetNumberCounter++;
  (*fragment)->type = newType;
  return true;
}

void* DPAUCS_getFragmentData( DPAUCS_fragment_t* fragment ){
  return (char*)fragment + DPAUCS_MEMPOOL_SIZE( fragment ) - fragment->size;
}

DPAUCS_fragment_t** DPAUCS_createFragment( enum DPAUCS_fragmentType type, size_t size ){
  size_t fragmentTypeSize = DPAUCS_getFragmentTypeSize( type );
  unsigned short packetNumber = packetNumberCounter++;
  DPAUCS_eachFragment(DPAUCS_ANY_FRAGMENT,&removeFragmentByPacketNumber,&packetNumber);
  if( fragmentsUsed < DPAUCS_MAX_FRAGMANTS )
    DPAUCS_removeOldestFragment();
  unsigned short i;
  for(i=DPAUCS_MAX_FRAGMANTS;i--;)
    if(!fragments[i])
      break;
  if(i>=DPAUCS_MAX_FRAGMANTS)
    return 0;
  DPAUCS_fragment_t** fragment_ptr = fragments + i;
  while( !DPAUCS_mempool_alloc(&mempool,(void**)fragment_ptr,size + fragmentTypeSize) && fragmentsUsed )
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
  DPAUCS_eachFragment(DPAUCS_ANY_FRAGMENT,&findOldestFragment,&args);
  if(!args.oldestFragment)
    return;
  DPAUCS_removeFragment(args.oldestFragment);
}

void DPAUCS_removeFragment( DPAUCS_fragment_t** fragment ){
  void(*destructor)(DPAUCS_fragment_t**) = fragmentTypeInfos[(*fragment)->type]->destructor;
  if(destructor)
    (*destructor)(fragment);
  DPAUCS_mempool_free(&mempool,(void**)fragment);
  fragmentsUsed--;
}

struct eachFragmentArgs {
  enum DPAUCS_fragmentType type;
  bool(*handler)(DPAUCS_fragment_t**,void*);
  void* arg;
};

static bool eachFragment_helper(void** mem,void* arg){
  struct eachFragmentArgs* args = arg;
  DPAUCS_fragment_t** fragment_ptr = (DPAUCS_fragment_t**)mem;
  if( args->type==DPAUCS_ANY_FRAGMENT || args->type == (*fragment_ptr)->type )
    return (*args->handler)(fragment_ptr,args->arg);
  return true;
}

void DPAUCS_eachFragment( enum DPAUCS_fragmentType type, bool(*handler)(DPAUCS_fragment_t**,void*), void* arg ){
  struct eachFragmentArgs args = {type,handler,arg};
  DPAUCS_mempool_each( &mempool, &eachFragment_helper, &args );
}
