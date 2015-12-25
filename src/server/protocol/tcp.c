#include <stdio.h>
#include <string.h>
#include <DPA/UCS/utils.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/logger.h>
#include <DPA/UCS/adelay.h>
#include <DPA/UCS/service.h>
#include <DPA/UCS/checksum.h>
#include <DPA/UCS/binaryUtils.h>
#include <DPA/UCS/protocol/tcp.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>
#include <DPA/UCS/protocol/tcp_stack.h>
#include <DPA/UCS/protocol/tcp_ip_stack_memory.h>
#include <DPA/UCS/protocol/tcp_retransmission_cache.h>

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
static inline bool tcp_setState( transmissionControlBlock_t* tcb, TCP_state_t state );
static DPAUCS_tcp_transmission_t tcp_begin( DPAUCS_tcp_t* tcp );
static bool tcp_transmit( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG );
static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG );


static uint32_t ISS = 0;


static transmissionControlBlock_t* searchTCB( transmissionControlBlock_t* tcb ){
  transmissionControlBlock_t* start = transmissionControlBlocks;
  transmissionControlBlock_t* end = transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( transmissionControlBlock_t* it=start; it<end ;it++ )
    if(
      it->state != TCP_CLOSED_STATE &&
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
    if( it->state != TCP_CLOSED_STATE && it->currentId == id )
      return it;
  return 0;
}

static void removeTCB( transmissionControlBlock_t* tcb ){
  if( tcb->state == TCP_CLOSED_STATE )
    return;
  DPAUCS_arpCache_deregister( tcb->fromTo.destination );
  if(tcb->service->onclose)
    (*tcb->service->onclose)(tcb);
  tcp_setState(tcb,TCP_CLOSED_STATE);
  tcb->currentId = 0;
  DPAUCS_LOG("Connection removed.\n");
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
  } while( stcb->state != TCP_CLOSED_STATE && !tcp_connectionUnstable( stcb ) );
  i = j;
  removeTCB( stcb );
  *stcb = *tcb;
  return stcb;
}

void printFrame( DPAUCS_tcp_t* tcp ){
  uint16_t flags = btoh16( tcp->flags );
  DPAUCS_LOG( "TCP Packet:\n"
    "  source: " "%u" "\n"
    "  destination: " "%u" "\n"
    "  sequence: " "%u" "\n"
    "  acknowledgment: " "%u" "\n"
    "  dataOffset: " "%u" "\n"
    "  flags: %s %s %s %s %s %s %s %s %s\n"
    "  windowSize: " "%u" "\n"
    "  checksum: " "%u" "\n"
    "  urgentPointer: " "%u" "\n",
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
  uint16_t data_checksum = ~checksumOfStream( stream, length );

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

static inline bool tcp_setState( transmissionControlBlock_t* tcb, TCP_state_t state ){
  if( tcb->state == state )
    return false;
  static const char* stateNames[] = {TCP_STATES(DPAUCS_STRINGIFY)};
  DPAUCS_LOG("%s => %s\n",stateNames[tcb->state],stateNames[state]);
  tcb->state = state;
  return true;
}

inline uint8_t tcp_flaglength( uint32_t flags ){
  /* RFC 793  Page 26
   * > ...
   * > is achived by implicitly including some
   * > control flags in the sequence space
   * > ... 
   * > The segment length (SEG.LEN) includes both 
   * > data and sequence space occupying controls
   */
  return (bool)( flags & ( TCP_FLAG_SYN | TCP_FLAG_FIN ) );
}

static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG ){

  bool result;

  // calculate length of datas without tcp header
  DPAUCS_stream_skipEntry( transmission->stream );
  size_t datalen = DPAUCS_stream_getLength( transmission->stream );
  DPAUCS_stream_reverseSkipEntry( transmission->stream );

  // store segmenmt length regarding extra sequencenumber for SYN and FIN
  for( unsigned i=count; i--; )
    SEG[i].LEN = datalen + tcp_flaglength( SEG[i].flags );

  // add datas to retransmissioncache if necessary
  if( datalen ){
    cleanupCache(); // TODO: add some checks if it's necessary
    if(!addToCache( transmission, count, tcb, SEG )){
      DPAUCS_LOG("TCP Retransmission cache full.\n");
      // since I must be able to retransmit those datas if I send them, I mustn't transmit them
      result = false;
      goto cleanup;
    }
  }

  // transmit datas
  result = tcp_transmit( transmission, count, tcb, SEG );

 cleanup:
  DPAUCS_layer3_destroyTransmissionStream( transmission->stream );
  return result;
}

static bool tcp_transmit( DPAUCS_tcp_transmission_t* transmission, unsigned count, transmissionControlBlock_t** tcb, tcp_segment_t* SEG ){

  // get pointer to entry in stream which represents tcp header
  streamEntry_t* tcpHeaderEntry = DPAUCS_stream_getEntry( transmission->stream );
  DPAUCS_stream_skipEntry( transmission->stream ); // Skip tcp header

  size_t headersize = tcpHeaderEntry->size; // get size of TCP Header

  // save start of datas of stream
  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveReadOffset( &sros, transmission->stream );

  DPAUCS_LOG("tcp_end: send to %u endpoints\n",count);

  // For each endpoint/TCB/client
  for( unsigned i=0; i<count; i++ ){

    // Get maximum size of payload the underlaying protocol can handle
    size_t l3_max = ~0;
    DPAUCS_layer3_getPacketSizeLimit( tcb[i]->fromTo.destination->type, &l3_max );

    DPAUCS_LOG(
      "tcp_end: send tcb %u, SND.WND: %lu, SND.NXT: %lu, l3_max: %lu\n",
      i, (unsigned long)tcb[i]->SND.WND, (unsigned long)tcb[i]->SND.NXT, (unsigned long)l3_max
    );

    uint32_t flags = SEG[i].flags;
    uint32_t next = SEG[i].SEQ;
    tcb[i]->flags.ackAlreadySent = tcb[i]->flags.ackAlreadySent || flags & TCP_FLAG_ACK;

    uint32_t alreadySent = tcb[i]->SND.NXT - SEG[i].SEQ;
    // If SND.UNA is bigger than SEG.SEQ, some datas may already be acknowledged
    // Since those values are in range 0 <= x <= 2^32 and all results are x mod 2^32,
    // I can't tell if SND.UNA is bigger than SEG.SEQ directly. Since I can't have more datas
    // acknowledged than I sent, the amount of acknowledged datas, which is SND.UNA - SEG.SEQ mod 2^32,
    // must be smaller or equal to those datas already sent. Otherwise, datas from a previous segment
    // havn't yet been acknowledged, and none of the datas of the current segment are acknowledged.
    DPAUCS_LOG("SND.UNA: %lx, SEG[%u].SEQ: %lx\n",tcb[i]->SND.UNA,i,SEG[i].SEQ);
    uint32_t alreadyAcknowledged = tcb[i]->SND.UNA - SEG[i].SEQ;
    if( alreadyAcknowledged > alreadySent )
      alreadyAcknowledged = 0;

    size_t segmentLength = SEG[i].LEN;
    if( segmentLength > l3_max - headersize )
      segmentLength = l3_max - headersize;
    if( segmentLength < alreadyAcknowledged )
      continue;
    segmentLength -= alreadyAcknowledged;
    if( segmentLength > tcb[i]->SND.WND )
      segmentLength = tcb[i]->SND.WND;

    size_t data_length = segmentLength - tcp_flaglength( flags );
    size_t packet_length = data_length + headersize;

    tcp_from_tcb( transmission->tcp, tcb[i], SEG+i );
    DPAUCS_stream_seek( transmission->stream, alreadyAcknowledged );
    DPAUCS_stream_reverseSkipEntry( transmission->stream );
    streamEntry_t* entryBeforeData = DPAUCS_stream_getEntry( transmission->stream );
    DPAUCS_stream_swapEntries( tcpHeaderEntry, entryBeforeData );
    tcp_calculateChecksum( tcb[i], transmission->tcp, transmission->stream, packet_length );
    DPAUCS_layer3_transmit( transmission->stream, &tcb[i]->fromTo, PROTOCOL_TCP, packet_length );
    DPAUCS_LOG( "tcp_end: %u bytes sent, tcp checksum %x\n", (unsigned)packet_length, (unsigned)transmission->tcp->checksum );
    DPAUCS_stream_swapEntries( tcpHeaderEntry, entryBeforeData );

    next += segmentLength;
    if( tcb[i]->SND.NXT < next )
      tcb[i]->SND.NXT = next;

    flags &= TCP_FLAG_URG;

    DPAUCS_stream_restoreReadOffset( transmission->stream, &sros );

  }

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

  if(tcb->service->onreceive)
    (*tcb->service->onreceive)( tcb, payload, length );

}

static bool tcp_connectionUnstable( transmissionControlBlock_t* stcb ){
  return ( stcb->state == TCP_SYN_RCVD_STATE   )
      || ( stcb->state == TCP_SYN_SENT_STATE   )
      || ( stcb->state == TCP_TIME_WAIT_STATE  )
      || ( stcb->state == TCP_CLOSING_STATE    )
      || ( stcb->state == TCP_LAST_ACK_STATE   )
      || ( stcb->state == TCP_CLOSED_STATE     );
}

void tcb_from_tcp(
  transmissionControlBlock_t* tcb,
  DPAUCS_tcp_t* tcp,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to
){
  tcb->srcPort     = btoh16( tcp->destination );
  tcb->destPort    = btoh16( tcp->source );
  tcb->service     = DPAUCS_get_service( &to->logicAddress, btoh16( tcp->destination ), PROTOCOL_TCP );
  tcb->currentId   = id;
  tcb->next_length = 0;

  tcb->fragments.first = 0;
  tcb->fragments.last  = 0;
  tcb->SND.WND = 0;

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

  if( tcb->state == TCP_TIME_WAIT_STATE ){
    DPAUCS_LOG("Ignored possible retransmission in TCP_TIME_WAIT_STATE\n");
    return false;
  }

  uint16_t n = tcb->next_length - headerLength;
  void* payload = (char*)tcp + headerLength;

  SEG.LEN = n + tcp_flaglength( SEG.flags );

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
    SEG.SEQ + n + tcp_flaglength( SEG.flags ) - tcb->RCV.NXT < tcb->RCV.WND
  )) return false;

  uint16_t offset;
  tcp_get_recivewindow_data_offset( tcb, &SEG, &offset );

  if( n < offset && !( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ) )
    return false;

  if( SEG.flags & TCP_FLAG_ACK ){
    uint32_t ackrel = ACK - tcb->SND.UNA;
    uint32_t ackwnd = tcb->SND.NXT - tcb->SND.UNA;
    DPAUCS_LOG( "ackrel: %u ackwnd: %u\n", (unsigned)ackrel, (unsigned)ackwnd );
    if( ackrel > ackwnd )
      return false;
  }

  // Statechanges //

  tcb->SND.WND = btoh32( tcp->windowSize );
  tcb->flags.ackAlreadySent = false;

  {
    unsigned long RCV_NXT_old = tcb->RCV.NXT;
    tcb->RCV.NXT = SEG.SEQ + SEG.LEN;
    DPAUCS_LOG(
      "n: %lu, tcb->RCV.NXT: %lu => %lu\n",
      (unsigned long)n,
      (unsigned long)RCV_NXT_old,
      (unsigned long)tcb->RCV.NXT
    );
  }

  if( SEG.flags & TCP_FLAG_ACK ){
    tcb->SND.UNA = btoh32(tcp->acknowledgment);
    if( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ){
      DPAUCS_LOG("TCP Keep-Alive\n");
      tcp_segment_t segment = {
        .flags = TCP_FLAG_ACK,
        .SEQ = tcb->SND.NXT
      };
      tcp_sendNoData( 1, &tcb, &segment );
    }else switch( tcb->state ){
      case TCP_SYN_RCVD_STATE  : tcp_setState( tcb, TCP_ESTAB_STATE      ); break;
      case TCP_CLOSING_STATE   : tcp_setState( tcb, TCP_TIME_WAIT_STATE  ); break;
      case TCP_FIN_WAIT_1_STATE: tcp_setState( tcb, TCP_FIN_WAIT_2_STATE ); break;
      case TCP_LAST_ACK_STATE  : removeTCB( tcb ); break;
      default: break;
    }
  }

  if( SEG.flags & TCP_FLAG_FIN ){
    switch( tcb->state ){
      case TCP_ESTAB_STATE     : tcp_setState( tcb, TCP_CLOSE_WAIT_STATE ); goto eof_code;
      case TCP_FIN_WAIT_1_STATE: tcp_setState( tcb, TCP_CLOSING_STATE    ); goto eof_code;
      case TCP_FIN_WAIT_2_STATE: tcp_setState( tcb, TCP_TIME_WAIT_STATE  ); goto eof_code;
      eof_code: if(tcb->service->oneof)(*tcb->service->oneof)(tcb); break;
      default: break;
    }
  }

  if(n){

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

  // ack received datas if necessary //
  if( SEG.LEN && !tcb->flags.ackAlreadySent ){
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK,
      .SEQ = tcb->SND.NXT
    };
    tcp_sendNoData( 1, &tcb, &segment );
  }

  return true;
}

static bool tcp_processHeader(
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t length,
  DPAUCS_fragment_t** fragment,
  void* payload,
  bool last
){

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
    tcb.RCV.NXT = SEG.SEQ + 1;
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
      .SEQ = ISS
    };
    tcb.SND.NXT = ISS;
    tcb.SND.UNA = ISS;
    tcp_sendNoData( 1, (transmissionControlBlock_t*[]){&tcb}, &segment );
    return false;
  }

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
      DPAUCS_LOG( "Dublicate SYN or already opened connection detected.\n" );
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
      DPAUCS_LOG("0/%d TCBs left. To many opened connections.\n",TRANSMISSION_CONTROL_BLOCK_COUNT);
      return false;
    }
    if(stcb->service->onopen){
      if(!(*stcb->service->onopen)(stcb)){
        tcp_segment_t segment = {
          .flags = TCP_FLAG_ACK | TCP_FLAG_RST,
          .SEQ = ISS
        };
        stcb->SND.UNA = ISS;
        stcb->SND.NXT = ISS;
        tcp_sendNoData( 1, &stcb, &segment );
        DPAUCS_LOG( "tcb->service->onopen: connection rejected\n" );
        removeTCB(stcb);
        return false;
      }
    }
    stcb->SND.UNA = ISS;
    stcb->SND.NXT = ISS;
    stcb->SND.WND = btoh32( tcp->windowSize );
    stcb->RCV.WND = DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE;
    tcp_segment_t segment = {
      .flags = TCP_FLAG_ACK | TCP_FLAG_SYN,
      .SEQ = ISS
    };
    tcp_sendNoData( 1, &stcb, &segment );
    return last;
  }else if( !stcb ){
    DPAUCS_LOG( "Connection unavaiable.\n" );
    tcp_segment_t segment = {
      .flags = TCP_FLAG_RST,
      .SEQ = ACK
    };
    tcb.SND.NXT = ACK;
    tcb.SND.UNA = ACK;
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

  (void)offset; // unused

  transmissionControlBlock_t tcb_tmp;

  bool ret = true;

  uint32_t tmp_ch = (uint16_t)~checksum( payload, length );

  transmissionControlBlock_t* tcb = getTcbByCurrentId(id);
  if(tcb){
    if( tcb->next_length & 1 )
      tmp_ch = (uint16_t)( (uint16_t)( tmp_ch >> 8 ) & 0xFF ) | (uint16_t)( (uint16_t)( tmp_ch & 0xFF ) << 8 );
    tmp_ch += (uint32_t)tcb->checksum;
    tmp_ch  = (uint16_t)tmp_ch + (uint16_t)( tmp_ch >> 16 );
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
      tcb_from_tcp( &tcb_tmp, tcp, id, from, to );
      tcb_ptr = &tcb_tmp;
    }else{
      tcb_ptr = tcb;
    }
    tmp_ch += (uint16_t)~tcp_pseudoHeaderChecksum( tcb_ptr, tcp, tcb_ptr->next_length + length );
    uint16_t ch = (uint16_t)~( (uint16_t)tmp_ch + (uint16_t)( tmp_ch >> 16 ) );
    if(!ch){
      DPAUCS_LOG("checksum OK\n");
    }else{
      DPAUCS_LOG("bad checksum (%.4x)\n",(int)ch);
      return false;
    }
  }

  if(!tcb){
    ret = tcp_processHeader( id, from, to, length, fragment, payload, last );
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

bool DPAUCS_tcp_send( bool(*func)( DPAUCS_stream_t*, void* ), void** cids, size_t count, void* ptr ){
  if( count > TRANSMISSION_CONTROL_BLOCK_COUNT )
    return false;

  DPAUCS_tcp_t tcp;
  DPAUCS_tcp_transmission_t t = tcp_begin( &tcp );

  tcp_segment_t SEGs[count];

  for(size_t i=0;i<count;i++)
    SEGs[i] = (tcp_segment_t){
      .flags = TCP_FLAG_ACK,
      .SEQ = ((struct transmissionControlBlock*)cids[i])->SND.NXT
    };

  if(!(*func)(t.stream,ptr)){
    DPAUCS_layer3_destroyTransmissionStream( t.stream );
    return false;
  };

  return tcp_end( &t, count, (transmissionControlBlock_t**)cids, SEGs );
}

void DPAUCS_tcp_abord( void* cid ){
  transmissionControlBlock_t* tcb = cid;
  if( tcb->state == TCP_CLOSED_STATE )
    return;
  tcp_segment_t segment = {
    .flags = TCP_FLAG_RST | TCP_FLAG_ACK,
    .SEQ = tcb->SND.NXT
  };
  tcp_setState( tcb, TCP_CLOSED_STATE );
  tcp_sendNoData( 1, &tcb, &segment );
}

void DPAUCS_tcp_close( void* cid ){
  transmissionControlBlock_t* tcb = cid;
  if( tcb->state == TCP_ESTAB_STATE ){
    tcp_setState( tcb, TCP_FIN_WAIT_1_STATE );
  }else if( tcb->state == TCP_CLOSE_WAIT_STATE ){
    tcp_setState( tcb, TCP_LAST_ACK_STATE );
  }else return;
  tcp_segment_t segment = {
    .flags = TCP_FLAG_FIN | TCP_FLAG_ACK,
    .SEQ = tcb->SND.NXT
  };
  tcp_sendNoData( 1, &tcb, &segment );
}

static void tcp_receiveFailtureHandler( void* id ){
  (void)id;
  DPAUCS_LOG("-- tcp_reciveFailtureHandler | id: %p --\n",id);
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
