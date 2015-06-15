#include <stdio.h>
#include <service.h>
#include <checksum.h>
#include <binaryUtils.h>
#include <protocol/tcp.h>
#include <protocol/arp.h>
#include <server.h>

// All connections
transmissionControlBlock_t transmissionControlBlocks[ TEMPORARY_TRANSMISSION_CONTROL_BLOCK_COUNT + STATIC_TRANSMISSION_CONTROL_BLOCK_COUNT ];
// Connections in FINWAIT-2, LAST-ACK, CLOSEING, SYN-SENT or SYN-RCVD state
transmissionControlBlock_t* temporaryTransmissionControlBlocks = transmissionControlBlocks;
// Connections in ESTAB, FIN-WAIT-1 or CLOSE-WAIT state
transmissionControlBlock_t* staticTransmissionControlBlocks = transmissionControlBlocks + TEMPORARY_TRANSMISSION_CONTROL_BLOCK_COUNT;

static bool tcp_reciveHandler( void*, DPAUCS_address_t*, DPAUCS_address_t*, DPAUCS_beginTransmission, DPAUCS_endTransmission, uint16_t, uint16_t, void*, bool );
static void tcp_reciveFailtureHandler( void* );
static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* );
static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* );

static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* tcb ){
  transmissionControlBlock_t* start = transmissionControlBlocks;
  transmissionControlBlock_t* end = transmissionControlBlocks+sizeof(transmissionControlBlocks)/sizeof(*transmissionControlBlocks);
  for(transmissionControlBlock_t* it=start;it<end;it++)
    if(
      it->active &&
      it->srcPort == tcb->srcPort &&
      it->destPort == tcb->destPort &&
      it->service == tcb->service &&
      DPAUCS_compare_logicAddress( &it->srcAddr->logicAddress , &tcb->srcAddr->logicAddress ) &&
      DPAUCS_compare_logicAddress( &it->destAddr->logicAddress, &tcb->destAddr->logicAddress )
    ) return it;
  return 0;
}

static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* tcb ){
  static unsigned i = 0;
  tcb->destAddr = DPAUCS_arpCache_register( tcb->destAddr );
  transmissionControlBlock_t* stcb = temporaryTransmissionControlBlocks + ( i++ % TEMPORARY_TRANSMISSION_CONTROL_BLOCK_COUNT );
  if(stcb->active)
    DPAUCS_arpCache_deregister( stcb->destAddr );
  *stcb = *tcb;
  return stcb;
}

void hexdump( const unsigned char* x, unsigned s, unsigned y ){
  unsigned i;
  for(i=0;i<s;i++,x++)
    printf("%.2x%c",(int)*x,((i+1)%y)?' ':'\n');
  if((i%y)) printf("\n");
}

static void printFrame( DPAUCS_tcp_t* tcp ){
  uint16_t flags = btoh16( tcp->flags );
  printf(
    "source: " "%u" "\n"
    "destination: " "%u" "\n"
    "sequence: " "%u" "\n"
    "acknowledgment: " "%u" "\n"
    "dataOffset: " "%u" "\n"
    "flags: %s %s %s %s %s %s %s %s %s\n"
    "windowSize: " "%u" "\n"
    "checksum: " "%u" "\n"
    "urgentPointer: " "%u" "\n",
    (unsigned)btoh16(tcp->source),
    (unsigned)btoh16(tcp->destination),
    (unsigned)btoh32(tcp->sequence),
    (unsigned)btoh32(tcp->acknowledgment),
    (unsigned)((tcp->dataOffset>>2)&~3u),
    ( flags & TCP_FLAG_FIN ) ? "FIN" : "",
    ( flags & TCP_FLAG_SYN ) ? "SYN" : "",
    ( flags & TCP_FLAG_RST ) ? "RST" : "",
    ( flags & TCP_FLAG_PSH ) ? "PSH" : "",
    ( flags & TCP_FLAG_ACK ) ? "ACK" : "",
    ( flags & TCP_FLAG_URG ) ? "URG" : "",
    ( flags & TCP_FLAG_ECE ) ? "ECE" : "",
    ( flags & TCP_FLAG_CWR ) ? "CWR" : "",
    ( flags & TCP_FLAG_NS  ) ? "NS"  : "",
    (unsigned)btoh16(tcp->windowSize),
    (unsigned)btoh16(tcp->checksum),
    (unsigned)btoh16(tcp->urgentPointer)
  );
}

static bool tcp_reciveHandler( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_beginTransmission begin, DPAUCS_endTransmission end, uint16_t offset, uint16_t length, void* payload, bool last ){
  if( length < 40 )
    return false;
  (void)id;
  (void)from;
  (void)to;
  (void)begin;
  (void)end;
  (void)last;
  DPAUCS_tcp_t* tcp = payload;
  uint16_t headerLength = ( tcp->dataOffset >> 2 ) & ~3u;
  if( headerLength > length )
    return false;
  transmissionControlBlock_t tcb = {
    .active = true,
    .srcPort = btoh16(tcp->destination),
    .destPort = btoh16(tcp->source),
    .srcAddr = to,
    .destAddr = from,
    .service = DPAUCS_get_service( &to->logicAddress, btoh16(tcp->destination) )
  };
  if(
      !tcb.service
   || !tcb.destPort
   || !DPAUCS_isValidHostAddress( &tcb.destAddr->logicAddress )
   || !tcb.service
  ) return false;

  printf("-- tcp_reciveHandler | id: %p offset: %u size: %u --\n",id,(unsigned)offset,(unsigned)length);

  transmissionControlBlock_t* stcb = searchTCB( &tcb );
  uint16_t flags = btoh16( tcp->flags );
  if( flags & TCP_FLAG_SYN ){
    if( stcb ){
      printf( "Dublicate SYN or already opened connection detected.\n" );
      return false;
    }
    tcb.state = TCP_SYN_RCVD_STATE;
    stcb = addTemporaryTCB( &tcb );
  }

  printFrame(tcp);

  uint8_t* options = (uint8_t*)payload + 40;
  while( options < (uint8_t*)payload+headerLength ){
  printf("option %.2X\n",(int)options[0]);
  switch(options[0]){
    case 0: goto afterOptionLoop; // end
    case 1: options++; continue; // noop
    default: { // other
      uint8_t size = options[1];
      if( size < 2 || options+size > (uint8_t*)payload+headerLength )
        goto afterOptionLoop;
      printf("size: %u\n",(unsigned)size);
      hexdump(options+2,size-2,16);
      options += size;
    } break;
  }
  }
  afterOptionLoop:;
  return true;
}

static void tcp_reciveFailtureHandler( void* id ){
  printf("-- tcp_reciveFailtureHandler | id: %p --\n",id);
}

static DPAUCS_layer3_protocolHandler_t tcp_handler = {
  .protocol = PROTOCOL_TCP,
  .onrecive = &tcp_reciveHandler,
  .onrecivefailture = &tcp_reciveFailtureHandler
};

static int counter = 0;

void DPAUCS_tcpInit(){
  if(counter++) return;
  DPAUCS_layer3_addProtocolHandler(&tcp_handler);
}

void DPAUCS_tcpShutdown(){
  if(--counter) return;
  DPAUCS_layer3_removeProtocolHandler(&tcp_handler);
}