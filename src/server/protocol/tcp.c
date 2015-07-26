#include <stdio.h>
#include <string.h>
#include <server.h>
#include <service.h>
#include <checksum.h>
#include <binaryUtils.h>
#include <protocol/tcp.h>
#include <protocol/arp.h>
#include <protocol/IPv4.h>
#include <protocol/tcp_retransmission_cache.h>

DPAUCS_MODUL( tcp ){}

transmissionControlBlock_t transmissionControlBlocks[ TRANSMISSION_CONTROL_BLOCK_COUNT ];


static bool tcp_reciveHandler( void*, DPAUCS_address_t*, DPAUCS_address_t*, DPAUCS_createTransmissionStream, DPAUCS_transmit, DPAUCS_destroyTransmissionStream, uint16_t, uint16_t, void*, bool );
static void tcp_reciveFailtureHandler( void* );
static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* );
static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* );
static void removeTCB( transmissionControlBlock_t* tcb );
static void tcp_from_tcb( DPAUCS_tcp_t* tcp, transmissionControlBlock_t* tcb, tcp_segment_t* SEG );
static void tcp_calculateChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream );
static transmissionControlBlock_t* getTcbByCurrentId(const void*const);
static bool tcp_sendNoData( unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG, DPAUCS_createTransmissionStream, DPAUCS_transmit, DPAUCS_destroyTransmissionStream );
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

static void tcp_from_tcb( DPAUCS_tcp_t* tcp, transmissionControlBlock_t* tcb, tcp_segment_t* SEG ){
  memset( tcp, 0, sizeof(*tcp) );
  tcp->destination = htob16( tcb->destPort );
  tcp->source = htob16( tcb->srcPort );
  tcp->acknowledgment = btoh32( tcb->RCV.NXT );
  tcp->sequence = htob32( SEG->SEQ );
  tcp->windowSize = htob16( tcb->RCV.WND );
  tcp->flags = htob16( ( ( sizeof( *tcp ) / 4 ) << 12 ) | SEG->flags );
}

#ifdef USE_IPv4
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
#endif

static uint16_t tcp_pseudoHeaderChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream ){
  switch( tcb->fromTo.source->type ){
#ifdef USE_IPv4
    case AT_IPv4: return tcp_IPv4_pseudoHeaderChecksum(tcb,tcp,stream);
#endif
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

inline uint32_t tcp_SEG_LEN( uint32_t datalen, uint32_t flags ){
  /* RFC 793  Page 26
   * > ...
   * > is achived by implicitly including some
   * > control flags in the sequence space
   * > ... 
   * > The segment length (SEG.LEN) includes both 
   * > data and sequqence space occupying controls
   */
  if( flags & ( TCP_FLAG_SYN | TCP_FLAG_FIN ) )
    return datalen + 1;
  else
    return datalen;
}

static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream ){

  cleanupCache(); // TODO: add some checks if it's necessary
  if(!addToCache( transmission, count, tcb, SEG ))
    return false;

  for(unsigned i=0;i<count;i++){
    tcp_from_tcb( transmission->tcp, tcb[i], SEG+i );
    tcp_calculateChecksum( tcb[i], transmission->tcp, transmission->stream );
    (*transmit)(transmission->stream,&tcb[i]->fromTo, PROTOCOL_TCP);
    uint32_t next = SEG[i].SEQ + tcp_SEG_LEN( DPAUCS_stream_getLength( transmission->stream ) - sizeof(DPAUCS_tcp_t), SEG[i].flags );
    if( tcb[i]->SND.NXT < next )
      tcb[i]->SND.NXT = next;
  }
  (*destroyStream)(transmission->stream);

  return true;
}

static bool tcp_sendNoData( unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG, DPAUCS_createTransmissionStream createStream, DPAUCS_transmit transmit, DPAUCS_destroyTransmissionStream destroyStream ){

  DPAUCS_tcp_t tcp;
  DPAUCS_tcp_transmission_t t = tcp_begin( &tcp, createStream );
  return tcp_end( &t, count, tcb, SEG, transmit, destroyStream );

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
  printf("\n-- data begin --\n");
  fwrite( payload, 1, length, stdout );
  printf("\n-- data end --\n");
  return true;
}

static bool tcp_connectionUnstable( transmissionControlBlock_t* stcb ){
  return ( stcb->state == TCP_SYN_RCVD_STATE   )
      || ( stcb->state == TCP_SYN_SENT_STATE   )
      || ( stcb->state == TCP_FIN_WAIT_2_STATE )
      || ( stcb->state == TCP_CLOSING_STATE    )
      || ( stcb->state == TCP_LAST_ACK_STATE   );
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

  tcp_segment_t SEG = {
    .ACK = btoh32(tcp->acknowledgment),
    .SEQ = btoh32(tcp->sequence),
    .flags = btoh16( tcp->flags )
  };

  transmissionControlBlock_t tcb = {
    .active = true,
    .srcPort = btoh16(tcp->destination),
    .destPort = btoh16(tcp->source),
    .fromTo = {
      .source = to,
      .destination = from
    },
    .RCV = {
      .NXT = SEG.SEQ + tcp_SEG_LEN( length - headerLength, SEG.flags )
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
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
      .ACK = tcb.RCV.NXT,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){&tcb}, &segment, createStream, transmit, destroyStream );
    return false;
  }

  printf("-- tcp_reciveHandler | id: %p offset: %u size: %u --\n",id,(unsigned)offset,(unsigned)length);

  enum {
    SEG_POS_LAST_OCTET,
    SEG_POS_NEXT_OCTET,
    SEG_POS_FUTUR_OCTET
  } segment_position = SEG_POS_NEXT_OCTET;

  transmissionControlBlock_t* stcb = searchTCB( &tcb );
  if( stcb ){
    // relSeq == 1 and no data is allowed as specified by
    // RFC 763  Page 25/26
    uint32_t relSeq = stcb->RCV.NXT - SEG.SEQ;
    if( relSeq == 1 )
      segment_position = SEG_POS_LAST_OCTET;
    else if( relSeq == 0 )
      segment_position = SEG_POS_NEXT_OCTET;
    else
      segment_position = SEG_POS_FUTUR_OCTET;
    printf("relSeq: %u\n",(unsigned)relSeq);
    switch( segment_position ){
      case SEG_POS_NEXT_OCTET: {
        printf("RCV.NXT: %u => %u\n",(unsigned)stcb->RCV.NXT,(unsigned)tcb.RCV.NXT);
        stcb->RCV.NXT = tcb.RCV.NXT;
      } break;
      case SEG_POS_LAST_OCTET:
        if( length==headerLength )
          break;
      case SEG_POS_FUTUR_OCTET:
      default:
        printf("Out of order: SEG.SEQ = %u\n",(unsigned)SEG.SEQ);
        return false;
    }
  }
  if( SEG.flags & TCP_FLAG_SYN ){
    if( stcb ){
      printf( "Dublicate SYN or already opened connection detected.\n" );
      return false;
    }
    tcb.state = TCP_SYN_RCVD_STATE;
    /* In case of a SYN with data,
     * I won't buffer a syn with data, however, 
     * I can acknowledge the SYN only, 
     * causeing a retransmission of all datas later or a RST
     */
    tcb.RCV.NXT = SEG.SEQ + 1;
    stcb = addTemporaryTCB( &tcb );
    if(!stcb)
      return false;
    stcb->SND.UNA = ISS + 1;
    stcb->SND.NXT = ISS + 1;
    stcb->RCV.WND = DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE;
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_SYN,
      .ACK = stcb->RCV.NXT,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, &stcb, &segment, createStream, transmit, destroyStream );
  }else if( !stcb ){
    printf( "Connection unavaiable.\n" );
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
      .ACK = tcb.RCV.NXT,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){&tcb}, &segment, createStream, transmit, destroyStream );
    return false;
  }

  printFrame( tcp );

  if( SEG.flags & TCP_FLAG_ACK ){
    uint32_t ackrel = SEG.ACK - stcb->SND.UNA;
    uint32_t ackwnd = stcb->SND.NXT - stcb->SND.UNA;
    printf( "ackrel: %u ackwnd: %u\n", (unsigned)ackrel, (unsigned)ackwnd );
    if( ackrel > ackwnd )
      return false;
    stcb->SND.UNA = btoh32(tcp->acknowledgment);
    switch( segment_position ){
      case SEG_POS_NEXT_OCTET: {
        if( stcb->state == TCP_SYN_RCVD_STATE ){
          stcb->state = TCP_ESTAB_STATE;
          printf("Connection established.\n");
        }
      } break;
      case SEG_POS_LAST_OCTET: {
        printf("TCP Keep-Alive\n");
        tcp_segment_t segment = {
          .flags = TCP_FLAG_ACK,
          .ACK = stcb->RCV.NXT,
          .SEQ = stcb->SND.NXT
        };
        tcp_sendNoData( 1, &stcb, &segment, createStream, transmit, destroyStream );
      } break;
      case SEG_POS_FUTUR_OCTET:
      default: break;
    }
  }

  if( SEG.flags & TCP_FLAG_FIN ){
    
  }

  /*
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
*/
  if( tcp_connectionUnstable(stcb)
   || headerLength >= length
   || segment_position != SEG_POS_NEXT_OCTET
  ){
    return !last;
  }else{
    return tcp_processDatas( stcb, createStream, transmit, destroyStream, offset+headerLength, length-headerLength, (char*)payload+headerLength, last );
  }
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