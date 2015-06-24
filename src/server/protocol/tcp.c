#include <stdio.h>
#include <packed.h>
#include <string.h>
#include <server.h>
#include <service.h>
#include <checksum.h>
#include <binaryUtils.h>
#include <protocol/tcp.h>
#include <protocol/arp.h>
#include <protocol/IPv4.h>


transmissionControlBlock_t transmissionControlBlocks[ TRANSMISSION_CONTROL_BLOCK_COUNT ];


static bool tcp_reciveHandler( void*, DPAUCS_address_t*, DPAUCS_address_t*, DPAUCS_createTransmissionStream, DPAUCS_transmit, DPAUCS_destroyTransmissionStream, uint16_t, uint16_t, void*, bool );
static void tcp_reciveFailtureHandler( void* );
static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* );
static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* );
static void removeTCB( transmissionControlBlock_t* tcb );
static void tcp_from_tcb( DPAUCS_tcp_t* tcp, transmissionControlBlock_t* tcb, uint16_t flags );
static void tcp_calculateChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream );
static transmissionControlBlock_t* getTcbByCurrentId(const void*const);
static void tcp_sendNoData( transmissionControlBlock_t** tcb, uint16_t flags, DPAUCS_createTransmissionStream, DPAUCS_transmit, DPAUCS_destroyTransmissionStream );
static bool tcp_connectionUnstable( transmissionControlBlock_t* stcb );


static uint32_t ISS = 0;


static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* tcb ){
  transmissionControlBlock_t* start = transmissionControlBlocks;
  transmissionControlBlock_t* end = transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( transmissionControlBlock_t* it=start; it<end ;it++ )
    if(
      it->active &&
      it->srcPort == tcb->srcPort &&
      it->destPort == tcb->destPort &&
      it->service == tcb->service &&
      DPAUCS_compare_logicAddress( &it->fromTo.source->logicAddress , &tcb->fromTo.source->logicAddress ) &&
      DPAUCS_compare_logicAddress( &it->fromTo.destination->logicAddress, &tcb->fromTo.destination->logicAddress )
    ) return it;
  return 0;
}

static transmissionControlBlock_t* getTcbByCurrentId( const void*const id ){
  transmissionControlBlock_t* start = transmissionControlBlocks;
  transmissionControlBlock_t* end = transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( transmissionControlBlock_t* it=start; it<end; it++ )
    if( it->active && it->currentId == id )
      return it;
  return 0;
}

static void removeTCB( transmissionControlBlock_t* tcb ){
  if( !tcb->active )
    return;
  DPAUCS_arpCache_deregister( tcb->fromTo.destination );
  tcb->active = false;
  tcb->currentId = 0;
}

static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* tcb ){
  static unsigned i = 0;
  unsigned j = i;
  DPAUCS_address_t* addr = DPAUCS_arpCache_register( tcb->fromTo.destination );
  if( !addr )
    return 0;
  tcb->fromTo.destination = addr;
  transmissionControlBlock_t* stcb;
  do {
    if( j >= i+TRANSMISSION_CONTROL_BLOCK_COUNT )
      return 0;
    stcb = transmissionControlBlocks + ( j++ % TRANSMISSION_CONTROL_BLOCK_COUNT );
  } while( stcb->active && !tcp_connectionUnstable( stcb ) );
  i = j;
  removeTCB( stcb );
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

static void tcp_from_tcb( DPAUCS_tcp_t* tcp, transmissionControlBlock_t* tcb, uint16_t flags ){
  memset( tcp, 0, sizeof(*tcp) );
  tcp->destination = btoh16( tcb->destPort );
  tcp->source = btoh16( tcb->srcPort );
  tcp->acknowledgment = btoh32( tcb->RCV.NXT );
  tcp->sequence = btoh32( tcb->SND.NXT );
  tcp->flags = btoh16( ( ( sizeof( *tcp ) / 4 ) << 12 ) | flags );
}

static uint16_t tcp_IPv4_pseudoHeaderChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream ){
  PACKED1 struct PACKED2 pseudoHeader {
    uint32_t src, dst;
    uint8_t padding, protocol;
    uint16_t length;
  };
  struct pseudoHeader pseudoHeader = {
    .src = htob32( ((DPAUCS_logicAddress_IPv4_t*)(&tcb->fromTo.source->logicAddress))->address ),
    .dst = htob32( ((DPAUCS_logicAddress_IPv4_t*)(&tcb->fromTo.destination->logicAddress))->address ),
    .padding = 0,
    .protocol = PROTOCOL_TCP,
    .length = htob16( DPAUCS_stream_getLength( stream ) )
  };
  (void)tcp;
  return checksum( &pseudoHeader, sizeof(pseudoHeader) );
}

static uint16_t tcp_pseudoHeaderChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream ){
  switch( tcb->fromTo.source->type ){
    case AT_IPv4: return tcp_IPv4_pseudoHeaderChecksum(tcb,tcp,stream);
  }
  return 0;
}

static void tcp_calculateChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream ){
  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveReadOffset( &sros, stream );

  uint16_t ps_checksum   = ~tcp_pseudoHeaderChecksum( tcb, tcp, stream );
  uint16_t data_checksum = ~checksumOfStream( stream );

  uint32_t tmp_checksum = (uint32_t)data_checksum + ps_checksum;
  uint16_t checksum = ~( ( tmp_checksum & 0xFFFF ) + ( tmp_checksum >> 16 ) );

  tcp->checksum = checksum;

  DPAUCS_stream_restoreReadOffset( stream, &sros );
}

static DPAUCS_tcp_transmission_t tcp_begin( DPAUCS_tcp_t* tcp, DPAUCS_createTransmissionStream createStream ){

  DPAUCS_stream_t* stream = (*createStream)();
  DPAUCS_stream_referenceWrite( stream, tcp, sizeof(*tcp) );

  return (DPAUCS_tcp_transmission_t){
    .tcp = tcp,
    .stream = stream
  };

}

static void tcp_end( DPAUCS_tcp_transmission_t* transmission, transmissionControlBlock_t** tcb, uint16_t flags, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream ){

  for(;*tcb;tcb++){
    tcp_from_tcb( transmission->tcp, *tcb, flags );
    tcp_calculateChecksum( *tcb, transmission->tcp, transmission->stream );
    (*transmit)(transmission->stream,&(*tcb)->fromTo, PROTOCOL_TCP);
    (*tcb)->SND.UNA = (*tcb)->SND.NXT;
    (*tcb)->SND.NXT += DPAUCS_stream_getLength( transmission->stream ) - sizeof(DPAUCS_tcp_t) + 1;
  }
  (*destroyStream)(transmission->stream);

}

static void tcp_sendNoData( transmissionControlBlock_t** tcb, uint16_t flags, DPAUCS_createTransmissionStream createStream, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream ){

  DPAUCS_tcp_t tcp;
  DPAUCS_tcp_transmission_t t = tcp_begin( &tcp, createStream );
  tcp_end( &t, tcb, flags, transmit, destroyStream );

}

static bool tcp_processDatas(
  transmissionControlBlock_t* tcb,
  DPAUCS_createTransmissionStream createStream,
  DPAUCS_transmit transmit,
  DPAUCS_destroyTransmissionStream destroyStream, 
  uint16_t offset,
  uint16_t length,
  void* payload,
  bool last
){
  (void)tcb;
  (void)createStream;
  (void)transmit;
  (void)destroyStream;
  (void)offset;
  (void)length;
  (void)payload;
  (void)last;
  return true;
}

static bool tcp_connectionUnstable( transmissionControlBlock_t* stcb ){
  return ( stcb->state & TCP_SYN_RCVD_STATE   )
      || ( stcb->state & TCP_SYN_SENT_STATE   )
      || ( stcb->state & TCP_FIN_WAIT_2_STATE )
      || ( stcb->state & TCP_CLOSING_STATE    )
      || ( stcb->state & TCP_LAST_ACK_STATE   );
}

static bool tcp_processHeader( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_createTransmissionStream createStream, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream, uint16_t offset, uint16_t length, void* payload, bool last ){
  ISS += length;
  if( length < sizeof(DPAUCS_tcp_t) )
    return false;

  (void)id;
  (void)last;

  DPAUCS_tcp_t* tcp = payload;

  uint16_t headerLength = ( tcp->dataOffset >> 2 ) & ~3u;
  if( headerLength > length )
    return false;

  transmissionControlBlock_t tcb = {
    .active = true,
    .srcPort = btoh16(tcp->destination),
    .destPort = btoh16(tcp->source),
    .fromTo = {
      .source = to,
      .destination = from
    },
    .SND = {
      .UNA = ISS,
      .NXT = ISS
    },
    .RCV = {
      .NXT = btoh32(tcp->sequence) + length - headerLength + 1,
    },
    .service = DPAUCS_get_service( &to->logicAddress, btoh16( tcp->destination ) ),
    .currentId = id
  };

  if(
      !tcb.service
   || !tcb.destPort
   || !DPAUCS_isValidHostAddress( &tcb.fromTo.destination->logicAddress )
   || !tcb.service
  ){
    tcp_sendNoData( (transmissionControlBlock_t*[]){&tcb,0}, TCP_FLAG_ACK | TCP_FLAG_RST, createStream, transmit, destroyStream );
    return false;
  }

  printf("-- tcp_reciveHandler | id: %p offset: %u size: %u --\n",id,(unsigned)offset,(unsigned)length);

  transmissionControlBlock_t* stcb = searchTCB( &tcb );
  uint16_t flags = btoh16( tcp->flags );
  if( flags & TCP_FLAG_SYN ){
    if( headerLength < length ){
      // Datas in SYNs are rarely seen, won't support them.
      printf( "SYN with datas rejected.\n" );
      tcp_sendNoData( (transmissionControlBlock_t*[]){&tcb,0}, TCP_FLAG_ACK | TCP_FLAG_RST, createStream, transmit, destroyStream );
      return false;
    }
    if( stcb ){
      printf( "Dublicate SYN or already opened connection detected.\n" );
      return false;
    }
    tcb.state = TCP_SYN_RCVD_STATE;
    stcb = addTemporaryTCB( &tcb );
    if(!stcb)
      return false;
    tcp_sendNoData( (transmissionControlBlock_t*[]){stcb,0}, TCP_FLAG_ACK | TCP_FLAG_SYN, createStream, transmit, destroyStream );
  }else if( !stcb ){
    printf( "Connection unavaiable.\n" );
    tcp_sendNoData( (transmissionControlBlock_t*[]){&tcb,0}, TCP_FLAG_ACK | TCP_FLAG_RST, createStream, transmit, destroyStream );
    return false;
  }

  if( flags & TCP_FLAG_ACK ){
    uint32_t ackrel = btoh32(tcp->acknowledgment) - stcb->SND.UNA - 1;
    uint32_t ackwnd = stcb->SND.NXT - stcb->SND.UNA;
    if( ackrel >= ackwnd )
      return false;
    stcb->SND.UNA = btoh32(tcp->acknowledgment);
    if( stcb->state == TCP_SYN_RCVD_STATE ){
      stcb->state = TCP_ESTAB_STATE;
      printf("Connection established.\n");
    }
  }

  if( flags & TCP_FLAG_FIN ){
    
  }

  printFrame( tcp );

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

  if( tcp_connectionUnstable(stcb) )
    return !last;
  else if( headerLength < length )
    tcp_processDatas( stcb, createStream, transmit, destroyStream, offset+headerLength, length-headerLength, (char*)payload+headerLength, last );

  return true;
}

static bool tcp_reciveHandler( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_createTransmissionStream createStream, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream, uint16_t offset, uint16_t length, void* payload, bool last ){

  bool ret;

  transmissionControlBlock_t* tcb = getTcbByCurrentId(id);
  if(tcb)
    ret = tcp_processDatas( tcb, createStream, transmit, destroyStream, offset, length, payload, last );
  else
    ret = tcp_processHeader( id, from, to, createStream, transmit, destroyStream, offset, length, payload, last );

  if(last){
    tcb = getTcbByCurrentId(id);
    if(tcb)
      tcb->currentId = 0;
  }

  return ret;
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