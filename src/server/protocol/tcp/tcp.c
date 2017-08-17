#include <stdio.h>
#include <string.h>

#define DPAUCS_TCP_C

#include <DPA/UCS/server.h>
#include <DPA/UCS/adelay.h>
#include <DPA/UCS/service.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/checksum.h>
#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>
#include <DPA/UCS/protocol/tcp/tcp_stack.h>
#include <DPA/UCS/protocol/tcp_ip_stack_memory.h>
#include <DPA/UCS/protocol/tcp/tcp_retransmission_cache.h>

DPA_MODULE( tcp ){
  DPA_DEPENDENCY( adelay_driver );
}

DPAUCS_transmissionControlBlock_t DPAUCS_transmissionControlBlocks[ TRANSMISSION_CONTROL_BLOCK_COUNT ];

static bool tcp_receiveHandler( void*, DPAUCS_address_t*, DPAUCS_address_t*, uint16_t, uint16_t, DPAUCS_fragment_t**, void*, bool );
static void tcp_receiveFailtureHandler( void* );
static DPAUCS_transmissionControlBlock_t* searchTCB( DPAUCS_transmissionControlBlock_t* );
static DPAUCS_transmissionControlBlock_t* addTemporaryTCB( DPAUCS_transmissionControlBlock_t* );
static void removeTCB( DPAUCS_transmissionControlBlock_t* tcb );
void tcp_from_tcb( DPAUCS_tcp_t* tcp, DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_segment_t* SEG );
void tcp_calculateChecksum( DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPA_stream_t* stream, uint16_t header_length, uint16_t length );
static DPAUCS_transmissionControlBlock_t* getTcbByCurrentId( const void*const );
static bool tcp_sendNoData( unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags );
static bool tcp_connectionUnstable( DPAUCS_transmissionControlBlock_t* stcb );
static inline bool tcp_setState( DPAUCS_transmissionControlBlock_t* tcb, enum DPAUCS_TCP_state state );
static DPAUCS_tcp_transmission_t tcp_begin( void );
bool tcp_transmit( DPAUCS_tcp_transmission_t* transmission, unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags, size_t* size, uint32_t* SEQs );
static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags );


static uint32_t ISS = 0;

static inline void tcp_una_change_handler( DPAUCS_transmissionControlBlock_t* tcb, uint32_t new_una ){
  tcb->SND.UNA = new_una;
  tcp_cacheCleanupTCB( tcb );
}

static DPAUCS_transmissionControlBlock_t* searchTCB( DPAUCS_transmissionControlBlock_t* tcb ){
  DPAUCS_transmissionControlBlock_t* start = DPAUCS_transmissionControlBlocks;
  DPAUCS_transmissionControlBlock_t* end = DPAUCS_transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( DPAUCS_transmissionControlBlock_t* it=start; it<end ; it++ )
    if( it->state != TCP_CLOSED_STATE
     && it->srcPort == tcb->srcPort
     && it->destPort == tcb->destPort
     && DPAUCS_mixedPairEqual( &it->fromTo , &tcb->fromTo )
    ) return it;
  return 0;
}

static DPAUCS_transmissionControlBlock_t* getTcbByCurrentId( const void*const id ){
  DPAUCS_transmissionControlBlock_t* start = DPAUCS_transmissionControlBlocks;
  DPAUCS_transmissionControlBlock_t* end = DPAUCS_transmissionControlBlocks + TRANSMISSION_CONTROL_BLOCK_COUNT;
  for( DPAUCS_transmissionControlBlock_t* it=start; it<end; it++ )
    if( it->state != TCP_CLOSED_STATE && it->currentId == id )
      return it;
  return 0;
}

static void removeTCB( DPAUCS_transmissionControlBlock_t* tcb ){
  if( tcb->state == TCP_CLOSED_STATE )
    return;
  if(tcb->service->onclose)
    (*tcb->service->onclose)(tcb);
  tcp_setState(tcb,TCP_CLOSED_STATE);
  DPAUCS_freeMixedAddress( &tcb->fromTo );
  tcb->currentId = 0;
  DPA_LOG("Connection removed.\n");
}

static DPAUCS_transmissionControlBlock_t* addTemporaryTCB( DPAUCS_transmissionControlBlock_t* tcb ){
  static unsigned i = 0;
  unsigned j = i;
  if( !DPAUCS_persistMixedAddress( &tcb->fromTo ) )
    return 0;
  DPAUCS_transmissionControlBlock_t* stcb;
  do {
    if( j >= i+TRANSMISSION_CONTROL_BLOCK_COUNT )
      return 0;
    stcb = DPAUCS_transmissionControlBlocks + ( j++ % TRANSMISSION_CONTROL_BLOCK_COUNT );
  } while( stcb->state != TCP_CLOSED_STATE && !tcp_connectionUnstable( stcb ) );
  i = j;
  removeTCB( stcb );
  *stcb = *tcb;
  return stcb;
}

void printFrame( DPAUCS_tcp_t* tcp ){
  uint16_t flags = DPA_btoh16( tcp->flags );
  DPA_LOG( "TCP Packet:\n"
    "  source: " "%u" "\n"
    "  destination: " "%u" "\n"
    "  sequence: " "%u" "\n"
    "  acknowledgment: " "%u" "\n"
    "  dataOffset: " "%u" "\n"
    "  flags: %s %s %s %s %s %s %s %s %s\n"
    "  windowSize: " "%u" "\n"
    "  checksum: " "%u" "\n"
    "  urgentPointer: " "%u" "\n",
    (unsigned)DPA_btoh16(tcp->source),
    (unsigned)DPA_btoh16(tcp->destination),
    (unsigned)DPA_btoh32(tcp->sequence),
    (unsigned)DPA_btoh32(tcp->acknowledgment),
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
    (unsigned)DPA_btoh16(tcp->windowSize),
    (unsigned)DPA_btoh16(tcp->checksum),
    (unsigned)DPA_btoh16(tcp->urgentPointer)
  );
}

void tcp_from_tcb( DPAUCS_tcp_t* tcp, DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_segment_t* SEG ){
  memset( tcp, 0, sizeof(*tcp) );
  tcp->destination = DPA_htob16( tcb->destPort );
  tcp->source = DPA_htob16( tcb->srcPort );
  tcp->acknowledgment = DPA_btoh32( tcb->RCV.NXT );
  tcp->sequence = DPA_htob32( SEG->SEQ );
  tcp->windowSize = DPA_htob16( tcb->RCV.WND );
  tcp->flags = DPA_htob16( ( ( sizeof( *tcp ) / 4 ) << 12 ) | SEG->flags );
}

static uint16_t tcp_pseudoHeaderChecksum( DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, uint16_t length ){
  (void)tcp;
  DPAUCS_logicAddress_pair_t fromTo = {0};
  DPAUCS_mixedPairToLogicAddress( &fromTo, &tcb->fromTo );
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( DPAUCS_mixedPairGetType( &tcb->fromTo ) );
  if( handler && handler->calculatePseudoHeaderChecksum )
    return (*handler->calculatePseudoHeaderChecksum)( fromTo.source, fromTo.destination, PROTOCOL_TCP, length );
  return 0;
}

void tcp_calculateChecksum( DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_t* tcp, DPA_stream_t* stream, uint16_t header_length, uint16_t length ){
  size_t stream_size = stream ? DPA_stream_getLength(stream,~0,0) : 0;

  uint16_t ps_checksum   = ~tcp_pseudoHeaderChecksum( tcb, tcp, length );
  uint16_t tcph_checksum = ~checksum( tcp, sizeof(*tcp) ); // TODO (option size)
  uint16_t data_checksum = ~checksumOfStream( stream, length-header_length );

  uint32_t tmp_checksum = (uint32_t)data_checksum + ps_checksum + tcph_checksum;
  uint16_t checksum = ~( ( tmp_checksum & 0xFFFF ) + ( tmp_checksum >> 16 ) );

  tcp->checksum = checksum;

  if(stream)
    DPA_stream_restoreReadOffset( stream, stream_size );
}

static DPAUCS_tcp_transmission_t tcp_begin( void ){
  return (DPAUCS_tcp_transmission_t){
    .stream = DPAUCS_layer3_createTransmissionStream()
  };
}

static inline bool tcp_setState( DPAUCS_transmissionControlBlock_t* tcb, enum DPAUCS_TCP_state state ){
  if( tcb->state == state )
    return false;
  if( state == TCP_ESTAB_STATE ){
    tcb->cache.flags.SYN = false;
  }
  static const char* stateNames[] = {TCP_STATES(DPA_STRINGIFY)};
  DPA_LOG("%s => %s\n",stateNames[tcb->state],stateNames[state]);
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

static bool tcp_end( DPAUCS_tcp_transmission_t* transmission, unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags ){

  bool result = tcp_addToCache( transmission, count, tcb, flags );

  if( !result )
    DPA_LOG("TCP Retransmission cache full.\n");

  DPAUCS_layer3_destroyTransmissionStream( transmission->stream );
  return result;

}

bool DPAUCS_tcp_transmit(
  DPA_stream_t* stream,
  DPAUCS_transmissionControlBlock_t* tcb,
  uint16_t flags,
  size_t data_size,
  uint32_t SEQ
){
  DPAUCS_tcp_t tcp;
  memset(&tcp,0,sizeof(tcp));

  const bool RST = flags & TCP_FLAG_RST;
  const bool ACK = flags & TCP_FLAG_ACK;
  if( RST ) flags = 0;
  const bool SYN = flags & TCP_FLAG_SYN;
  const bool FIN = flags & TCP_FLAG_FIN;

  if(RST)
    stream = 0;

  size_t headersize = sizeof(tcp); // TODO (option size)
  size_t stream_size = stream ? DPA_stream_getLength(stream,~0,0) : 0;

  // Get maximum size of payload the underlaying protocol can handle
  size_t l3_max = ~0;
  DPAUCS_layer3_getPacketSizeLimit( DPAUCS_mixedPairGetType( &tcb->fromTo ), &l3_max );

  DPA_LOG(
    "DPAUCS_tcp_transmit: send tcb | SND.WND: %lu, SND.NXT: %lu, l3_max: %lu\n",
    (unsigned long)tcb->SND.WND, (unsigned long)tcb->SND.NXT, (unsigned long)l3_max
  );

  uint32_t next = SEQ;
  uint32_t offset = 0;
  uint32_t size, off;

  do {

    flags = ACK ? TCP_FLAG_ACK : 0;

    size = data_size;

    uint32_t alreadySent = tcb->SND.NXT - SEQ;
      // If SND.UNA is bigger than SEG.SEQ, some datas may already be acknowledged
      // Since those values are in range 0 <= x <= 2^32 and all results are x mod 2^32,
      // I can't tell if SND.UNA is bigger than SEG.SEQ directly. Since I can't have more datas
      // acknowledged than I sent, the amount of acknowledged datas, which is SND.UNA - SEG.SEQ mod 2^32,
      // must be smaller or equal to those datas already sent. Otherwise, datas from a previous segment
      // havn't yet been acknowledged, and none of the datas of the current segment are acknowledged.
    DPA_LOG("SND.UNA: %lx, SEG.SEQ: %lx\n",(unsigned long)tcb->SND.UNA,(unsigned long)SEQ);
    uint32_t alreadyAcknowledged = tcb->SND.UNA - SEQ;
    if( alreadyAcknowledged > alreadySent )
      alreadyAcknowledged = 0;

    off = alreadyAcknowledged + offset;
    if( SYN ){
      if( off ){
        off -= 1; // Remove SEQ for SYN
      }else{
        flags |= TCP_FLAG_SYN; // Set SYN flag
      }
    }

    if(RST)
      flags |= TCP_FLAG_RST;

    // Check if there is anything to send, even if it's only an ACK
    if( size + SYN + FIN < off + !ACK )
      break;

    size -= off;
    // Ensure datas will fit into packet
    if( size > l3_max - headersize )
      size = l3_max - headersize;
    // Ensure datas will fit into window
    if( size > tcb->SND.WND - offset ){
      size = tcb->SND.WND - offset;
      if( !size && ( SYN || FIN ) )
        break;
      if( size < (uint32_t)SYN + FIN ){
        size = 0;
      }else{
        size -= SYN + FIN;
      }
    }
    if( FIN && size == data_size - off )
      flags |= TCP_FLAG_FIN;

    size_t packet_length = size + headersize;

    DPAUCS_tcp_segment_t tmp_segment = {
      .LEN = size + tcp_flaglength(flags),
      .SEQ = SEQ + alreadyAcknowledged + offset,
      .flags = flags
    };
    tcp_from_tcb( &tcp, tcb, &tmp_segment );
    if( stream && off && !offset )
      DPA_stream_seek( stream, off );
    tcp_calculateChecksum( tcb, &tcp, stream, headersize, packet_length );
    printFrame(&tcp);
    DPAUCS_layer3_transmit( 1, (const size_t[]){headersize}, (const void*[]){&tcp}, stream, &tcb->fromTo, PROTOCOL_TCP, size );
    DPA_LOG( "DPAUCS_tcp_transmit: %u bytes sent, tcp checksum %x\n", (unsigned)packet_length, (unsigned)tcp.checksum );

    size_t segSize = size + tcp_flaglength(flags);
    offset += segSize;
    next += segSize;
    if( tcb->SND.NXT < next )
      tcb->SND.NXT = next;

  } while( size < off );

  if( stream && stream_size )
    DPA_stream_restoreReadOffset( stream, stream_size );

  return true;
}

static bool tcp_sendNoData( unsigned count, DPAUCS_transmissionControlBlock_t** tcb, uint16_t* flags ){

  DPAUCS_tcp_transmission_t t = tcp_begin();
  return tcp_end( &t, count, tcb, flags );

}

static void tcp_processDatas(
  DPAUCS_transmissionControlBlock_t* tcb,
  void* payload,
  size_t length
){

  if(tcb->service->onreceive)
    (*tcb->service->onreceive)( tcb, payload, length );

}

static bool tcp_connectionUnstable( DPAUCS_transmissionControlBlock_t* stcb ){
  return ( stcb->state == TCP_SYN_RCVD_STATE   )
      || ( stcb->state == TCP_SYN_SENT_STATE   )
      || ( stcb->state == TCP_TIME_WAIT_STATE  )
      || ( stcb->state == TCP_CLOSING_STATE    )
      || ( stcb->state == TCP_LAST_ACK_STATE   )
      || ( stcb->state == TCP_CLOSED_STATE     );
}

void tcb_from_tcp(
  DPAUCS_transmissionControlBlock_t* tcb,
  DPAUCS_tcp_t* tcp,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to
){
  memset(tcb,0,sizeof(*tcb));

  tcb->srcPort     = DPA_btoh16( tcp->destination );
  tcb->destPort    = DPA_btoh16( tcp->source );
  tcb->service     = DPAUCS_get_service( &to->logic, DPA_btoh16( tcp->destination ), PROTOCOL_TCP );
  tcb->currentId   = id;
  tcb->next_length = 0;

  tcb->SND.UNA = ISS;
  tcb->SND.NXT = ISS;

  DPAUCS_address_pair_t ap = {
    .source      = to,
    .destination = from
  };
  DPAUCS_addressPairToMixed( &tcb->fromTo, &ap );
}

static inline void tcp_init_variables(
  DPAUCS_tcp_t* tcp,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  DPAUCS_tcp_segment_t* SEG,
  DPAUCS_transmissionControlBlock_t* tcb,
  uint32_t* ACK
){

  SEG->SEQ   = DPA_btoh32( tcp->sequence );
  SEG->flags = DPA_btoh16( tcp->flags );

  tcb_from_tcp(tcb,tcp,id,from,to);

  *ACK = DPA_btoh32( tcp->acknowledgment );

}

static inline bool tcp_is_tcb_valid( DPAUCS_transmissionControlBlock_t* tcb ){
  DPAUCS_logicAddress_pair_t lap;
  DPAUCS_mixedPairToLogicAddress( &lap, &tcb->fromTo );
  return tcb->service
      && tcb->destPort
      && DPAUCS_isValidHostAddress( lap.destination );
}

static inline bool tcp_get_receivewindow_data_offset( DPAUCS_transmissionControlBlock_t* tcb, DPAUCS_tcp_segment_t* SEG, uint16_t* offset ){
  uint32_t off = tcb->RCV.NXT - SEG->SEQ;
  *offset = off;
  return off + 1 <= tcb->RCV.WND;
}

static bool tcp_processPacket(
  DPAUCS_transmissionControlBlock_t* tcb,
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t last_length,
  void* last_payload
){

  // Initialisation and checks //

  DPAUCS_tcp_segment_t SEG;
  DPAUCS_transmissionControlBlock_t tmp_tcb;

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

  SEG.SEQ   = DPA_btoh32( tcp->sequence );
  SEG.flags = DPA_btoh16( tcp->flags );

  uint32_t ACK = DPA_btoh32( tcp->acknowledgment );

  if(!tcb){
    tcb_from_tcp(&tmp_tcb,tcp,id,from,to);
    tcb = searchTCB( &tmp_tcb );
    if(!tcb)
      return false;
    if( tcb->currentId && tcb->currentId != tmp_tcb.currentId )
      return false;
  }

  if( tcb->state == TCP_TIME_WAIT_STATE ){
    DPA_LOG("Ignored possible retransmission in TCP_TIME_WAIT_STATE\n");
    return false;
  }

  uint16_t n = tcb->next_length - headerLength;
  void* payload = (char*)tcp + headerLength;

  SEG.LEN = n + tcp_flaglength( SEG.flags );

  if( !tcp_is_tcb_valid( tcb ) ){
    tcp_sendNoData( 1, (DPAUCS_transmissionControlBlock_t*[]){tcb}, (uint16_t[]){ TCP_FLAG_ACK | TCP_FLAG_RST });
    return false;
  }

  if(!(
    SEG.SEQ - tcb->RCV.NXT < tcb->RCV.WND ||
    SEG.SEQ + n + tcp_flaglength( SEG.flags ) - tcb->RCV.NXT < tcb->RCV.WND
  )) return false;

  uint16_t offset;
  tcp_get_receivewindow_data_offset( tcb, &SEG, &offset );

  if( n < offset && !( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ) )
    return false;

  if( SEG.flags & TCP_FLAG_ACK ){
    uint32_t ackrel = ACK - tcb->SND.UNA;
    uint32_t ackwnd = tcb->SND.NXT - tcb->SND.UNA;
    DPA_LOG( "ackrel: %u ackwnd: %u\n", (unsigned)ackrel, (unsigned)ackwnd );
    if( ackrel > ackwnd )
      return false;
  }

  // Statechanges //

  tcb->SND.WND = DPA_btoh32( tcp->windowSize );

  {
    unsigned long RCV_NXT_old = tcb->RCV.NXT;
    tcb->RCV.NXT = SEG.SEQ + SEG.LEN;
    DPA_LOG(
      "n: %lu, tcb->RCV.NXT: %lu => %lu\n",
      (unsigned long)n,
      (unsigned long)RCV_NXT_old,
      (unsigned long)tcb->RCV.NXT
    );
  }

  if( SEG.flags & TCP_FLAG_ACK ){
    tcp_una_change_handler( tcb, DPA_btoh32( tcp->acknowledgment ) );
    if( n==0 && SEG.SEQ == tcb->RCV.NXT-1 ){
      DPA_LOG("TCP Keep-Alive\n");
      tcp_sendNoData( 1, &tcb, (uint16_t[]){ TCP_FLAG_ACK });
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
  if( SEG.LEN )
    tcp_sendNoData( 1, &tcb, (uint16_t[]){ TCP_FLAG_ACK });

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

  DPAUCS_tcp_segment_t SEG;
  DPAUCS_transmissionControlBlock_t tcb;
  uint32_t ACK;

  tcp_init_variables( tcp, id, from, to, &SEG, &tcb, &ACK );

  if( !tcp_is_tcb_valid( &tcb ) ){
    tcb.RCV.NXT = SEG.SEQ + 1;
    tcp_sendNoData( 1, (DPAUCS_transmissionControlBlock_t*[]){&tcb}, (uint16_t[]){ TCP_FLAG_ACK | TCP_FLAG_RST });
    return false;
  }

  DPAUCS_transmissionControlBlock_t* stcb = searchTCB( &tcb );
  if( stcb ){
    if( stcb->currentId && stcb->currentId != tcb.currentId )
      return false;
    stcb->currentId = tcb.currentId;
    uint16_t rcv_data_offset;
    if( tcp_get_receivewindow_data_offset( stcb, &SEG, &rcv_data_offset ) )
      return false; // out of window
    // may be in window
    if(!last)
      return DPAUCS_tcp_cache_add( fragment, stcb );
  }

  if( SEG.flags & TCP_FLAG_SYN ){
    if( stcb ){
      DPA_LOG( "Dublicate SYN or already opened connection detected.\n" );
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
      DPA_LOG("0/%d TCBs left. To many opened connections.\n",TRANSMISSION_CONTROL_BLOCK_COUNT);
      return false;
    }
    if(stcb->service->onopen){
      if(!(*stcb->service->onopen)(stcb)){
        tcp_sendNoData( 1, &stcb, (uint16_t[]){ TCP_FLAG_ACK | TCP_FLAG_RST });
        DPA_LOG( "tcb->service->onopen: connection rejected\n" );
        removeTCB(stcb);
        return false;
      }
    }
    stcb->SND.WND = DPA_btoh32( tcp->windowSize );
    stcb->RCV.WND = DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE;
    tcp_sendNoData( 1, &stcb, (uint16_t[]){ TCP_FLAG_ACK | TCP_FLAG_SYN });
    return last;
  }else if( !stcb ){
    DPA_LOG( "Connection unavaiable.\n" );
    tcb.SND.NXT = ACK;
    tcb.SND.UNA = ACK;
    tcp_sendNoData( 1, (DPAUCS_transmissionControlBlock_t*[]){&tcb}, (uint16_t[]){ TCP_FLAG_RST });
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

  DPAUCS_transmissionControlBlock_t tcb_tmp;

  bool ret = true;

  uint32_t tmp_ch = (uint16_t)~checksum( payload, length );

  DPAUCS_transmissionControlBlock_t* tcb = getTcbByCurrentId(id);
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
    DPAUCS_transmissionControlBlock_t* tcb_ptr;
    if(!tcb){
      tcb_from_tcp( &tcb_tmp, tcp, id, from, to );
      tcb_ptr = &tcb_tmp;
    }else{
      tcb_ptr = tcb;
    }
    tmp_ch += (uint16_t)~tcp_pseudoHeaderChecksum( tcb_ptr, tcp, tcb_ptr->next_length + length );
    uint16_t ch = (uint16_t)~( (uint16_t)tmp_ch + (uint16_t)( tmp_ch >> 16 ) );
    if(!ch){
      DPA_LOG("checksum OK\n");
    }else{
      DPA_LOG("bad checksum (%.4x)\n",(int)ch);
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

bool DPAUCS_tcp_send( bool(*func)( DPA_stream_t*, void* ), void** cids, size_t count, void* ptr ){
  if( count > TRANSMISSION_CONTROL_BLOCK_COUNT )
    return false;
  {
    DPAUCS_tcp_transmission_t t = tcp_begin();

    uint16_t flags[count];
    for(size_t i=0;i<count;i++)
      flags[i] = TCP_FLAG_ACK;

    if(!(*func)(t.stream,ptr)){
      DPAUCS_layer3_destroyTransmissionStream( t.stream );
      return false;
    };

    return tcp_end( &t, count, (DPAUCS_transmissionControlBlock_t**)cids, flags );
  }
}

void DPAUCS_tcp_abord( void* cid ){
  DPAUCS_transmissionControlBlock_t* tcb = cid;
  if( tcb->state == TCP_CLOSED_STATE )
    return;
  tcp_setState( tcb, TCP_CLOSED_STATE );
  tcp_sendNoData( 1, &tcb, (uint16_t[]){ TCP_FLAG_RST | TCP_FLAG_ACK });
}

void DPAUCS_tcp_close( void* cid ){
  DPAUCS_transmissionControlBlock_t* tcb = cid;
  if( tcb->state == TCP_ESTAB_STATE ){
    tcp_setState( tcb, TCP_FIN_WAIT_1_STATE );
  }else if( tcb->state == TCP_CLOSE_WAIT_STATE ){
    tcp_setState( tcb, TCP_LAST_ACK_STATE );
  }else return;
  tcp_sendNoData( 1, &tcb, (uint16_t[]){ TCP_FLAG_FIN | TCP_FLAG_ACK });
}

static void tcp_receiveFailtureHandler( void* id ){
  (void)id;
  DPA_LOG("-- tcp_receiveFailtureHandler | id: %p --\n",id);
}

static DPAUCS_l4_handler_t tcpHandler = {
  .protocol = PROTOCOL_TCP,
  .onreceive = &tcp_receiveHandler,
  .onreceivefailture = &tcp_receiveFailtureHandler
};

static int counter = 0;

DPAUCS_INIT( tcp ){
  if(counter++) return;
  DPAUCS_layer3_addProtocolHandler(&tcpHandler);
}

DPAUCS_SHUTDOWN( tcp ){
  if(--counter) return;
  DPAUCS_layer3_removeProtocolHandler(&tcpHandler);
}

void tcp_do_next_task( void ){
  tcp_retransmission_cache_do_retransmissions();
}
