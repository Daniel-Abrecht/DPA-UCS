#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/protocol/ip_stack.h>
#include <DPA/UCS/protocol/tcp/tcp_stack.h>

static DPAUCS_fragmentHandler_t fragment_handler;

bool DPAUCS_tcp_cache_add( DPAUCS_fragment_t** fragment, DPAUCS_transmissionControlBlock_t* tcb ){
  if(! DPAUCS_takeover( fragment, &fragment_handler ) )
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
  for( DPAUCS_tcp_fragment_t** it = tcb->fragments.first; it; it=(*it)->next )
    DPAUCS_removeFragment( (DPAUCS_fragment_t**)it );
  tcb->fragments.first = 0;
  tcb->fragments.last  = 0;
}

static void fragmentDestructor(DPAUCS_fragment_t** f){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  // TODO
}

static bool fragmentBeforeTakeover( DPAUCS_fragment_t*** f, DPAUCS_fragmentHandler_t* newHandler ){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  (void)newHandler;
  // TODO
  return true;
}

static void takeoverFailtureHandler(DPAUCS_fragment_t** f){
  DPAUCS_tcp_fragment_t** tcpf = (DPAUCS_tcp_fragment_t**)*f;
  (void)tcpf;
  // TODO
}


static DPAUCS_fragmentHandler_t fragment_handler = {
  .fragmentInfo_size = sizeof(DPAUCS_tcp_fragment_t),
  .destructor = &fragmentDestructor,
  .beforeTakeover = &fragmentBeforeTakeover,
  .takeoverFailtureHandler = &takeoverFailtureHandler
};

DPAUCS_EXPORT_FRAGMENT_HANDLER( TCP, &fragment_handler );
