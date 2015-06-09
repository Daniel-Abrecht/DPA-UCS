#include <string.h>
#include <stdio.h>
#include <server.h>
#include <checksum.h>
#include <binaryUtils.h>
#include <protocol/ethtypes.h>
#include <protocol/ip.h>
#include <protocol/ip_stack.h>

DPAUCS_ipProtocolHandler_t* ipProtocolHandlers[MAX_IP_PROTO_HANDLERS];

void DPAUCS_addIpProtocolHandler(DPAUCS_ipProtocolHandler_t* handler){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( !ipProtocolHandlers[i] ){
      ipProtocolHandlers[i] = handler;
      break;
    }
}

void DPAUCS_removeIpProtocolHandler(DPAUCS_ipProtocolHandler_t* handler){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( ipProtocolHandlers[i] == handler ){
      ipProtocolHandlers[i] = 0;
      break;
    }
}

//static buffer_buffer_t outputStreamBufferBuffer;
DEFINE_BUFFER(unsigned char,uchar_buffer_t,outputStreamBuffer,0);
DEFINE_BUFFER(bufferInfo_t,buffer_buffer_t,outputStreamBufferBuffer,3);

static stream_t outputStream = {
  &outputStreamBuffer,
  &outputStreamBufferBuffer,
  0,
  0
};

static struct {
  DPAUCS_address_t* from;
  DPAUCS_address_t** to;
  uint8_t type;
} currentTransmission;

static stream_t* layer3_transmissionBegin( DPAUCS_address_t* from, DPAUCS_address_t** to, uint8_t type ){
  currentTransmission.from = from;
  currentTransmission.to = to;
  currentTransmission.type = type;
  outputStream.outputStreamBufferWriteStartOffset = outputStreamBuffer.write_offset;
  outputStream.outputStreamBufferBufferWriteStartOffset = outputStreamBufferBuffer.write_offset;
  return &outputStream;
}

static void IPv4_transmit( stream_t* inputStream, const ip_address_t* src, const ip_address_t* dst ){
  const uint16_t hl = 5;
  static uint16_t id = 0;

  uint8_t offset = 0;

  while(!DPAUCS_stream_eof(inputStream)){

    // prepare next packet
    DPAUCS_packet_info p;
    memset( &p, 0, sizeof(DPAUCS_packet_info) );
    p.type = ETH_TYPE_IP_V4;
    memcpy( p.destination_mac, dst->addr.mac, 6 );
    memcpy( p.source_mac, src->addr.mac, 6 );
    DPAUCS_preparePacket( &p );

    // create ip header
    DPAUCS_ipv4_t* ip = p.payload;
    ip->version_ihl = (uint16_t)( 4u << 4u ) | ( hl & 0x0F );
    ip->id = htob16( id );
    ip->ttl = DEFAULT_TTL;
    ip->protocol = currentTransmission.type;
    ip->source = htob32( src->ip );
    ip->destination = htob32( dst->ip );

    // create content
    size_t s = DPAUCS_stream_read( inputStream, ((unsigned char*)p.payload) + hl * 4, (PACKET_MAX_PAYLOAD - sizeof(DPAUCS_ipv4_t)) & ~7u );

    // complete ip header
    uint8_t flags = 0;

    if(!DPAUCS_stream_eof(inputStream))
      flags |= IPv4_FLAG_MORE_FRAGMENTS;

    ip->length = htob16( p.size + hl * 4 + s );
    ip->flags_offset1 = ( flags << 5 ) | ( ( offset >> 11 ) & 0x1F );
    ip->offset2 = ( offset >> 3 ) & 0xFF;

    ip->checksum = 0;
    ip->checksum = checksum( p.payload, hl * 4 );

    // send packet
    DPAUCS_sendPacket( &p, hl * 4 + s );

    // store ip packet data offset
    offset += s;

  }

  id++;
}

static void layer3_transmissionEnd(){

  uchar_buffer_t cb = outputStreamBuffer;
  buffer_buffer_t bb = outputStreamBufferBuffer;
  stream_t inputStream = { &cb, &bb, 0, 0 };

  for( DPAUCS_address_t** to=currentTransmission.to; *to; to++ ){

    // reset read offsets
    cb.read_offset = outputStreamBuffer.read_offset;
    bb.read_offset = outputStreamBufferBuffer.read_offset;

    switch((*to)->type){
      case AT_IPv4: IPv4_transmit( &inputStream, (const ip_address_t*)currentTransmission.from, (const ip_address_t*)*to ); break;
    }

  }

  outputStreamBufferBuffer.read_offset = outputStreamBufferBuffer.write_offset;
  outputStreamBuffer.read_offset = outputStreamBuffer.write_offset;

}

static void IPv4_handler( DPAUCS_packet_info* info, DPAUCS_ipv4_t* ip ){
  if( btoh16(ip->length) > info->size )
    return;

  uint32_t source = btoh32(ip->source);
  uint32_t destination = btoh32(ip->destination);

  {
    DPAUCS_logicAddress_IPv4_t addr = {
      LA_IPv4_INIT,
      .address = destination
    };
    if( !DPAUCS_isValidHostAddress(&addr.logicAddress)
     || !DPAUCS_has_logicAddress(&addr.logicAddress)
    ) return;
  }

  DPAUCS_ipProtocolHandler_t* handler = 0;
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( ipProtocolHandlers[i]
     && ipProtocolHandlers[i]->protocol == ip->protocol
    ){
      handler = ipProtocolHandlers[i];
      break;
    }
  if(!handler) return; 

  uint16_t headerlength = (ip->version_ihl & 0x0F) * 4;

  if( checksum( ip, headerlength ) )
    return; // invalid checksum

  DPAUCS_ip_fragment_t fragment;
  DPAUCS_ipPacketInfo_t ipInfo;
  ipInfo.src.ip = source;
  ipInfo.dest.ip = destination;
  ipInfo.src.addr.type = AT_IPv4;
  ipInfo.dest.addr.type = AT_IPv4;
  memcpy(ipInfo.src.addr.mac,info->source_mac,6);
  memcpy(ipInfo.dest.addr.mac,info->destination_mac,6);
  ipInfo.id = ip->id;
  ipInfo.tos = ip->tos;
  ipInfo.offset = 0;
  ipInfo.valid = true;
  ipInfo.onremove = handler->onrecivefailture;

  fragment.offset = (uint16_t)( (uint16_t)( ip->flags_offset1 & 0x1F ) | (uint16_t)ip->offset2 << 5u ) * 8u;
  fragment.length = btoh16(ip->length) - headerlength;
  fragment.flags = ( ip->flags_offset1 >> 5 ) & 0x07;

  uint8_t* payload = ((uint8_t*)ip) + headerlength;

  DPAUCS_ipPacketInfo_t* ipi = DPAUCS_normalize_ip_packet_info_ptr(&ipInfo);
  bool isNext;
  if(ipi){
    fragment.info = ipi;
    isNext = DPAUCS_isNextIpFragment(&fragment);
  }else{
    isNext = !fragment.offset; 
  }

  if( fragment.flags & IPv4_FLAG_MORE_FRAGMENTS || !isNext ){
    if(!ipi)
      ipi = DPAUCS_save_ip_packet_info(&ipInfo);
    fragment.info = ipi;
    if(isNext){
      DPAUCS_updateIpPackatOffset(&fragment);
      DPAUCS_ip_fragment_t* f = &fragment;
      DPAUCS_ip_fragment_t** f_ptr = &f;
      while(true){
        if(
            !(*handler->onrecive)(
              ipi,
              &f->info->src.addr,
              &f->info->dest.addr,
              &layer3_transmissionBegin,
              &layer3_transmissionEnd,
              f->offset,
              f->length,
              payload,
              !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
            )
         || !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
        ){
          ipi->onremove = 0;
          DPAUCS_removeIpPacket(f->info);
        }else{
	  DPAUCS_removeIpFragment(f_ptr);
	}
        f_ptr = DPAUCS_searchFollowingIpFragment(*f_ptr);
        if(!f_ptr)
          break;
        f = *f_ptr;
	payload = (uint8_t*)(f+1);
      }
    }else{
      DPAUCS_ip_fragment_t** f_ptr = DPAUCS_allocIpFragment(ipi,fragment.length);
      if(!f_ptr)
        return;
      memcpy(
        ((uint8_t*)*f_ptr) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        ((uint8_t*)&fragment) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        sizeof(DPAUCS_ip_fragment_t) - DPAUCS_IP_FRAGMENT_DATA_OFFSET
      );
      memcpy((*f_ptr)+1,payload,fragment.length);
      return;
    }
  }else{
    DPAUCS_ipPacketInfo_t* ipp = ipi ? ipi : &ipInfo;
    (*handler->onrecive)(
      ipp,
      &ipp->src.addr,
      &ipp->dest.addr,
      &layer3_transmissionBegin,
      &layer3_transmissionEnd,
      fragment.offset,
      fragment.length,
      payload,
      !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
    );
    if(ipi){
      ipi->onremove = 0;
      DPAUCS_removeIpPacket(ipi);
    }
    ipi = 0;
  }

}

void DPAUCS_ip_handler( DPAUCS_packet_info* info ){
  DPAUCS_ip_t* ip = info->payload;
  switch( ip->version >> 4 ){
    case 4: IPv4_handler(info,&ip->ipv4); break;
  }
}

