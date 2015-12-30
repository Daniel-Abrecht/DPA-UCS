#include <DPA/UCS/protocol/tcp.h>
#include <DPA/UCS/protocol/tcp_stack.h>

bool DPAUCS_tcp_cache_add( DPAUCS_fragment_t** fragment, DPAUCS_transmissionControlBlock_t* tcb ){
  if(! DPAUCS_takeover( fragment, DPAUCS_FRAGMENT_TYPE_TCP ) )
    return false;
  (void)tcb;
  DPAUCS_tcp_fragment_t** tf = (DPAUCS_tcp_fragment_t**)fragment;
  (*tf)->next = 0;
  if( tcb->fragments.last )
    (*tcb->fragments.last)->next = tf;
  tcb->fragments.last = tf;
  if( !tcb->fragments.first )
    tcb->fragments.first = tf;
  return true;
}

void DPAUCS_tcp_cache_remove( DPAUCS_transmissionControlBlock_t* tcb ){
  for(DPAUCS_tcp_fragment_t** it=tcb->fragments.first;it;it=(*it)->next)
    DPAUCS_removeFragment( (DPAUCS_fragment_t**)it );
  tcb->fragments.first = 0;
  tcb->fragments.last  = 0;
}

static void fragmentDestructor(DPAUCS_fragment_t** f){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  // TODO
}

static bool fragmentBeforeTakeover( DPAUCS_fragment_t*** f, enum DPAUCS_fragmentType newType ){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  (void)newType;
  // TODO
  return true;
}

static void takeoverFailtureHandler(DPAUCS_fragment_t** f){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  // TODO
}


extern const DPAUCS_fragment_info_t DPAUCS_tcp_fragment_info;
const DPAUCS_fragment_info_t DPAUCS_tcp_fragment_info = {
  .destructor = &fragmentDestructor,
  .beforeTakeover = &fragmentBeforeTakeover,
  .takeoverFailtureHandler = &takeoverFailtureHandler
};
