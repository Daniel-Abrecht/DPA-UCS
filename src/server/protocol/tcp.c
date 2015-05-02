#include <stdio.h>
#include <checksum.h>
#include <protocol/tcp.h>

static void hexdump(const unsigned char* x,unsigned s,unsigned y){
  unsigned i;
  for(i=0;i<s;i++,x++)
    printf("%.2x%c",(int)*x,((i+1)%y)?' ':'\n');
  if((i%y)) printf("\n");
}

static bool tcp_reciveHandler( void* id, void* from, void* to, DPAUCS_beginTransmission begin, DPAUCS_endTransmission end, uint16_t offset, uint16_t length, void* payload, bool last ){
  (void)id;
  (void)from;
  (void)to;
  (void)begin;
  (void)end;
  (void)last;
  printf("-- tcp_reciveHandler | id: %p offset: %u size: %u --\n",id,(unsigned)offset,(unsigned)length);
  hexdump(payload,length,16);
  return true;
}

static void tcp_reciveFailtureHandler( void* id ){
  printf("-- tcp_reciveFailtureHandler | id: %p --\n",id);
}

static DPAUCS_ipProtocolHandler_t tcp_handler = {
  .protocol = IP_PROTOCOL_TCP,
  .onrecive = &tcp_reciveHandler,
  .onrecivefailture = &tcp_reciveFailtureHandler
};

static int counter = 0;

void DPAUCS_tcpInit(){
  if(counter++) return;
  DPAUCS_addIpProtocolHandler(&tcp_handler);
}

void DPAUCS_tcpShutdown(){
  if(--counter) return;
  DPAUCS_removeIpProtocolHandler(&tcp_handler);
}