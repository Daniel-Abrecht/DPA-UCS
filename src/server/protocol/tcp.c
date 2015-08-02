#include <string.h>
#include <server.h>
#include <adelay.h>
#include <service.h>
#include <checksum.h>
#include <binaryUtils.h>
#include <protocol/tcp.h>
#include <protocol/arp.h>
#include <protocol/IPv4.h>
#include <protocol/layer3.h>
#include <protocol/tcp_stack.h>
#include <protocol/tcp_ip_stack_memory.h>
#include <protocol/tcp_retransmission_cache.h>

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif


DPAUCS_MODUL( tcp ){
  DPAUCS_DEPENDENCY( adelay_driver );
}

transmissionControlBlock_t transmissionControlBlocks[ TRANSMISSION_CONTROL_BLOCK_COUNT ];


static bool tcp_receiveHandler( void*, DPAUCS_address_t*, DPAUCS_address_t*, uint16_t, uint16_t, DPAUCS_fragment_t**, void*, bool );
static void tcp_receiveFailtureHandler( void* );
static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* );
static transmissionControlBlock_t* addTemporaryTCB( transmissionControlBlock_t* );
static void removeTCB( transmissionControlBlock_t* tcb );
static void tcp_from_tcb( DPAUCS_tcp_t* tcp, transmissionControlBlock_t* tcb, tcp_segment_t* SEG );
static void tcp_calculateChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream, uint16_t length );
static transmissionControlBlock_t* getTcbByCurrentId( const void*const );
static bool tcp_sendNoData( unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG );
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
  DEBUG_PRINTF("Connection removed.\n");
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

#ifdef DEBUG
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
#else
#define printFrame(...)
#endif

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
static uint16_t tcp_IPv4_pseudoHeaderChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, uint16_t length ){
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
    .length = htob16( length )
  };
  (void)tcp;
  return checksum( &pseudoHeader, sizeof(pseudoHeader) );
}
#endif

static uint16_t tcp_pseudoHeaderChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, uint16_t length ){

  switch( tcb->fromTo.source->type ){
#ifdef USE_IPv4
    case AT_IPv4: return tcp_IPv4_pseudoHeaderChecksum(tcb,tcp,length);
#endif
  }
  return 0;
}

static void tcp_calculateChecksum( transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPAUCS_stream_t* stream, uint16_t length ){
  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveReadOffset( &sros, stream );

  uint16_t ps_checksum   = ~tcp_pseudoHeaderChecksum( tcb, tcp, length );
  uint16_t data_checksum = ~checksumOfStream( stream );

  uint32_t tmp_checksum = (uint32_t)data_checksum + ps_checksum;
  uint16_t checksum = ~( ( tmp_checksum & 0xFFFF ) + ( tmp_checksum >> 16 ) );

  tcp->checksum = checksum;

  DPAUCS_stream_restoreReadOffset( stream, &sros );
}

static DPAUCS_tcp_transmission_t tcp_begin( DPAUCS_tcp_t* tcp ){

  DPAUCS_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
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
   * > data and sequence space occupying controls
   */
  if( flags & ( TCP_FLAG_SYN | TCP_FLAG_FIN ) )
    return datalen + 1;
  else
    return datalen;
}

static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG ){

  size_t length = DPAUCS_stream_getLength( transmission->stream );
  uint16_t len_tmp = length - sizeof(DPAUCS_tcp_t);

  for(unsigned i=count;i--;)
    SEG[i].LEN = tcp_SEG_LEN( len_tmp, SEG[i].flags );

  if( len_tmp ){
    cleanupCache(); // TODO: add some checks if it's necessary
    if(!addToCache( transmission, count, tcb, SEG )){
      DEBUG_PRINTF("TCP Retransmission cache full.\n");
      return false;
    }
  }

  for(unsigned i=0;i<count;i++){
    tcp_from_tcb( transmission->tcp, tcb[i], SEG+i );
    tcp_calculateChecksum( tcb[i], transmission->tcp, transmission->stream, length );
    DPAUCS_layer3_transmit(transmission->stream,&tcb[i]->fromTo, PROTOCOL_TCP);
    uint32_t next = SEG[i].SEQ + SEG[i].LEN;
    if( tcb[i]->SND.NXT < next )
      tcb[i]->SND.NXT = next;
  }
  DPAUCS_layer3_destroyTransmissionStream(transmission->stream);

  return true;
}

static bool tcp_sendNoData( unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG ){

  DPAUCS_tcp_t tcp;
  DPAUCS_tcp_transmission_t t = tcp_begin( &tcp );
  return tcp_end( &t, count, tcb, SEG );

}

static void tcp_processDatas(
  transmissionControlBlock_t* tcb,
  void* payload,
  size_t length
){

  (*tcb->service->onreceive)( tcb, payload, length );

}

static bool tcp_connectionUnstable( transmissionControlBlock_t* stcb ){
  return ( stcb->state == TCP_SYN_RCVD_STATE   )
      || ( stcb->state == TCP_SYN_SENT_STATE   )
      || ( stcb->state == TCP_FIN_WAIT_2_STATE )
      || ( stcb->state == TCP_CLOSING_STATE    )
      || ( stcb->state == TCP_LAST_ACK_STATE   );
}

void tcb_from_tcp(
  transmissionControlBlock_t* tcb,
  DPAUCS_tcp_t* tcp,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to
){
  tcb->active      = true;
  tcb->srcPort     = btoh16( tcp->destination );
  tcb->destPort    = btoh16( tcp->source );
  tcb->service     = DPAUCS_get_service( &to->logicAddress, btoh16( tcp->destination ) );
  tcb->currentId   = id;
  tcb->next_length = 0;

  tcb->fragments.first = 0;
  tcb->fragments.last  = 0;

  tcb->fromTo.source      = to;
  tcb->fromTo.destination = from;
}

static inline void tcp_init_variables(
  DPAUCS_tcp_t* tcp,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  tcp_segment_t* SEG,
  transmissionControlBlock_t* tcb,
  uint32_t* ACK
){

  SEG->SEQ   = btoh32( tcp->sequence );
  SEG->flags = btoh16( tcp->flags );

  tcb_from_tcp(tcb,tcp,id,from,to);

  *ACK = btoh32( tcp->acknowledgment );

}

static inline bool tcp_is_tcb_valid( transmissionControlBlock_t* tcb ){
  return tcb->service
      && tcb->destPort
      && DPAUCS_isValidHostAddress( &tcb->fromTo.destination->logicAddress );
}

static inline bool tcp_get_recivewindow_data_offset( transmissionControlBlock_t* tcb, tcp_segment_t* SEG, uint16_t* offset ){
  uint32_t off = tcb->RCV.NXT - SEG->SEQ;
  *offset = off;
  return off + 1 <= tcb->RCV.WND;
}

static bool tcp_processPacket(
  transmissionControlBlock_t* tcb,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t last_length,
  void* last_payload
){

  // Initialisation and checks //

  tcp_segment_t SEG;
  transmissionControlBlock_t tmp_tcb;

  DPAUCS_tcp_t* tcp = last_payload;
  uint16_t chunck_len = last_length;

  if( tcb && tcb->fragments.first ){
    tcp = DPAUCS_getFragmentData(&(*tcb->fragments.first)->fragment);
    chunck_len = (*tcb->fragments.first)->fragment.size;
  }

  if( chunck_len < sizeof(DPAUCS_tcp_t) )
    return false;

  uint16_t headerLength = ( tcp->dataOffset >> 2 ) & ~3u;
  if( headerLength > chunck_len )
    return false;

  chunck_len -= headerLength;

  SEG.SEQ   = btoh32( tcp->sequence );
  SEG.flags = btoh16( tcp->flags );

  uint32_t ACK = btoh32( tcp->acknowledgment );

  if(!tcb){
    tcb_from_tcp(&tmp_tcb,tcp,id,from,to);
    tcb = searchTCB( &tmp_tcb );
    if(!tcb)
      return false;
    if( tcb->currentId && tcb->currentId != tmp_tcb.currentId )
      return false;
  }

  uint16_t n = tcb->next_length - headerLength;
  void* payload = (char*)tcp + headerLength;

  if( !tcp_is_tcb_valid( tcb ) ){
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){tcb}, &segment );
    return false;
  }

  if(!(
    SEG.SEQ - tcb->RCV.NXT < tcb->RCV.WND ||
    SEG.SEQ + tcp_SEG_LEN( n, SEG.flags ) - tcb->RCV.NXT < tcb->RCV.WND
  )) return false;

  uint16_t offset;
  tcp_get_recivewindow_data_offset( tcb, &SEG, &offset );

  if( n < offset && !( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ) )
    return false;

  if( SEG.flags & TCP_FLAG_ACK ){
    uint32_t ackrel = ACK - tcb->SND.UNA;
    uint32_t ackwnd = tcb->SND.NXT - tcb->SND.UNA;
    DEBUG_PRINTF( "ackrel: %u ackwnd: %u\n", (unsigned)ackrel, (unsigned)ackwnd );
    if( ackrel > ackwnd )
      return false;
  }

  // Statechanges //

  DEBUG_PRINTF("n: %lu\n",(unsigned long)n);
  DEBUG_PRINTF("tcb->RCV.NXT: %lu => ",(unsigned long)tcb->RCV.NXT);
  tcb->RCV.NXT = SEG.SEQ + tcp_SEG_LEN( n, SEG.flags );
  DEBUG_PRINTF("%lu\n",(unsigned long)tcb->RCV.NXT);

  if( SEG.flags & TCP_FLAG_ACK ){
    tcb->SND.UNA = btoh32(tcp->acknowledgment);
    if( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ){
      DEBUG_PRINTF("TCP Keep-Alive\n");
      tcp_segment_t segment = {
        .flags = TCP_FLAG_ACK,
        .SEQ = tcb->SND.NXT
      };
      tcp_sendNoData( 1, &tcb, &segment );
    }else{
      switch( tcb->state ){
        case TCP_SYN_RCVD_STATE: {
          tcb->state = TCP_ESTAB_STATE;
          DEBUG_PRINTF("Connection established.\n");
        } break;
        case TCP_CLOSING_STATE: {
          tcb->state = TCP_TIME_WAIT_STATE;
        } break;
        case TCP_LAST_ACK_STATE: {
          removeTCB( tcb );
        } break;
        case TCP_FIN_WAIT_1_STATE: {
          tcb->state = TCP_FIN_WAIT_2_STATE;
        } break;
        default: break;
      }
    }
  }

  if( SEG.flags & TCP_FLAG_FIN ){
    switch( tcb->state ){
      case TCP_ESTAB_STATE: {
        tcb->state = TCP_CLOSE_WAIT_STATE;
        tcp_segment_t segment = {
          .flags = TCP_FLAG_ACK,
          .SEQ = tcb->SND.NXT
        };
        tcp_sendNoData( 1, &tcb, &segment );
      } break;
      case TCP_FIN_WAIT_1_STATE: {
        tcb->state = TCP_CLOSING_STATE;
        tcp_segment_t segment = {
          .flags = TCP_FLAG_ACK,
          .SEQ = tcb->SND.NXT
        };
        tcp_sendNoData( 1, &tcb, &segment );
      } break;
      case TCP_FIN_WAIT_2_STATE: {
        tcb->state = TCP_TIME_WAIT_STATE;
        tcp_segment_t segment = {
          .flags = TCP_FLAG_ACK,
          .SEQ = tcb->SND.NXT
        };
        tcp_sendNoData( 1, &tcb, &segment );
      } break;
      default: break;
    }
  }

  // ack received datas //

  if(n){
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK,
      .SEQ = tcb->SND.NXT
    };
    tcp_sendNoData( 1, &tcb, &segment );
 
    // process received datas //
    DPAUCS_tcp_fragment_t** it = tcb->fragments.first;
    {

      prepare: {
        if( chunck_len <= offset ){
          offset -= chunck_len;
          goto next;
        }else{
          chunck_len -= offset;
          payload = (char*)payload + offset;
          offset = 0;
        }
        goto process;
      }

      process: {
        tcp_processDatas( tcb, payload, chunck_len );
      }

      next: {
        if(!it){
          goto end;
        }else{
          it=(*it)->next;
        }
        if(it){
          payload = DPAUCS_getFragmentData(&(*it)->fragment);
          chunck_len = (*it)->fragment.size;
        }else{
          payload = last_payload;
          chunck_len = last_length;
        }
        goto prepare;
      }

      end:;
    }

  }

  return true;
}

static bool tcp_processHeader(
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t offset,
  uint16_t length,
  DPAUCS_fragment_t** fragment,
  void* payload,
  bool last
){(void)offset;

  ISS += length;
  if( length < sizeof(DPAUCS_tcp_t) )
    return false;

  DPAUCS_tcp_t* tcp = payload;

  uint16_t headerLength = ( tcp->dataOffset >> 2 ) & ~3u;
  if( headerLength > length )
    return false;
  length -= headerLength;

  tcp_segment_t SEG;
  transmissionControlBlock_t tcb;
  uint32_t ACK;

  tcp_init_variables( tcp, id, from, to, &SEG, &tcb, &ACK );

  if( !tcp_is_tcb_valid( &tcb ) ){
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){&tcb}, &segment );
    return false;
  }

  DEBUG_PRINTF("-- tcp_reciveHandler | id: %p offset: %u size: %u --\n",id,(unsigned)offset,(unsigned)length);
  printFrame( tcp );

  transmissionControlBlock_t* stcb = searchTCB( &tcb );
  if( stcb ){
    if( stcb->currentId && stcb->currentId != tcb.currentId )
      return false;
    stcb->currentId = tcb.currentId;
    uint16_t rcv_data_offset;
    if( tcp_get_recivewindow_data_offset( stcb, &SEG, &rcv_data_offset ) )
      return false; // out of window
    // may be in window
    if(!last)
      return DPAUCS_tcp_cache_add( fragment, stcb );
  }

  if( SEG.flags & TCP_FLAG_SYN ){
    if( stcb ){
      DEBUG_PRINTF( "Dublicate SYN or already opened connection detected.\n" );
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
    if(!stcb){
      DEBUG_PRINTF("0/%d TCBs left. To many opened connections.\n",TRANSMISSION_CONTROL_BLOCK_COUNT);
      return false;
    }
    stcb->SND.UNA = ISS + 1;
    stcb->SND.NXT = ISS + 1;
    stcb->RCV.WND = DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE;
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_SYN,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, &stcb, &segment );
    return last;
  }else if( !stcb ){
    DEBUG_PRINTF( "Connection unavaiable.\n" );
    tcp_segment_t segment = {
      .flags = TCP_FLAG_RST,
      .SEQ = ACK
    };
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){&tcb}, &segment );
    return false;
  }

  return true;
}

static bool tcp_receiveHandler(
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t offset,
  uint16_t length,
  DPAUCS_fragment_t** fragment,
  void* payload,
  bool last
){

  bool ret = true;

  uint32_t tmp_ch = ~checksum( payload, length ) & 0xFFFF;

  transmissionControlBlock_t* tcb = getTcbByCurrentId(id);
  if(tcb){
    if( tcb->next_length & 1 )
      tmp_ch = ( ( tmp_ch >> 8 ) & 0xFF ) | ( ( tmp_ch & 0xFF ) << 8 );
    tmp_ch += (uint32_t)tcb->checksum;
    tmp_ch  = ( tmp_ch & 0xFFFF ) + ( ( tmp_ch >> 16 ) & 0xFFFF );
    if(!last)
      ret = DPAUCS_tcp_cache_add( fragment, tcb ) || last;
  }

  if(last){
    DPAUCS_tcp_t* tcp;
    if( tcb ){
      if(!tcb->fragments.first)
        return false;
      tcp = DPAUCS_getFragmentData( &(*tcb->fragments.first)->fragment );
    }else{
      if( length < sizeof(DPAUCS_tcp_t) )
        return false;
      tcp = payload;
    }
    transmissionControlBlock_t* tcb_ptr;
    if(!tcb){
      transmissionControlBlock_t tcb_tmp;
      tcb_from_tcp( &tcb_tmp, tcp, id, from, to );
      tcb_ptr = &tcb_tmp;
    }else{
      tcb_ptr = tcb;
    }
    tmp_ch += ~tcp_pseudoHeaderChecksum( tcb_ptr, tcp, tcb_ptr->next_length + length )  & 0xFFFF;
    uint16_t ch = ~( ( tmp_ch & 0xFFFF ) + ( tmp_ch >> 16 ) );
    if(!ch){
      DEBUG_PRINTF("\nchecksum OK\n\n");
    }else{
      DEBUG_PRINTF("\nbad checksum (%.4x)\n\n",(int)ch);
      return false;
    }
    //uint16_t ch = btoh16( tcp->checksum );
    //DEBUG_PRINTF("%x %x\n",res,(unsigned)ch);
  }

  if(!tcb){
    ret = tcp_processHeader( id, from, to, offset, length, fragment, payload, last );
  }

  tcb = getTcbByCurrentId(id);
  if(tcb)
    tcb->next_length += length;

  if(last)
    ret = tcp_processPacket( tcb, id, from, to, length, payload );

  if(tcb){
    if( last ){
      tcb->currentId = 0;
      tcb->checksum = 0;
      tcb->next_length = 0;
      DPAUCS_tcp_cache_remove( tcb );
    }else{
      tcb->checksum = tmp_ch;
    }
  }

  return ret;
}

static void tcp_receiveFailtureHandler( void* id ){
  (void)id;
  DEBUG_PRINTF("-- tcp_reciveFailtureHandler | id: %p --\n",id);
}

static DPAUCS_layer3_protocolHandler_t tcp_handler = {
  .protocol = PROTOCOL_TCP,
  .onreceive = &tcp_receiveHandler,
  .onreceivefailture = &tcp_receiveFailtureHandler
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

void tcp_do_next_task( void ){
  tcp_retransmission_cache_do_retransmissions();
}
