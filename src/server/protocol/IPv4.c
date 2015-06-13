#include <server.h>
#include <protocol/layer3.h>
#include <protocol/ip_stack.h>
#include <protocol/ethtypes.h>

void DPAUCS_IPv4_handler( DPAUCS_packet_info* info, DPAUCS_IPv4_t* ip ){
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

  DPAUCS_layer3_protocolHandler_t* handler = 0;
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( layer3_protocolHandlers[i]
     && layer3_protocolHandlers[i]->protocol == ip->protocol
    ){
      handler = layer3_protocolHandlers[i];
      break;
    }
  if(!handler) return; 

  uint16_t headerlength = (ip->version_ihl & 0x0F) * 4;

  if( checksum( ip, headerlength ) )
    return; // invalid checksum

  DPAUCS_IPv4_fragment_t fragment;
  DPAUCS_ipPacketInfo_t ipInfo;
  ipInfo.src.ip = source;
  ipInfo.dest.ip = destination;
  ipInfo.src.address.type = AT_IPv4;
  ipInfo.dest.address.type = AT_IPv4;
  memcpy(ipInfo.src.address.mac,info->source_mac,6);
  memcpy(ipInfo.dest.address.mac,info->destination_mac,6);
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
      DPAUCS_IPv4_fragment_t* f = &fragment;
      DPAUCS_IPv4_fragment_t** f_ptr = &f;
      while(true){
        if(
            !(*handler->onrecive)(
              ipi,
              &f->info->src.address,
              &f->info->dest.address,
              &DPAUCS_layer3_transmissionBegin,
              &DPAUCS_layer3_transmissionEnd,
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
      DPAUCS_IPv4_fragment_t** f_ptr = DPAUCS_allocIpFragment(ipi,fragment.length);
      if(!f_ptr)
        return;
      memcpy(
        ((uint8_t*)*f_ptr) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        ((uint8_t*)&fragment) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        sizeof(DPAUCS_IPv4_fragment_t) - DPAUCS_IP_FRAGMENT_DATA_OFFSET
      );
      memcpy((*f_ptr)+1,payload,fragment.length);
      return;
    }
  }else{
    DPAUCS_ipPacketInfo_t* ipp = ipi ? ipi : &ipInfo;
    (*handler->onrecive)(
      ipp,
      &ipp->src.address,
      &ipp->dest.address,
      &DPAUCS_layer3_transmissionBegin,
      &DPAUCS_layer3_transmissionEnd,
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

void DPAUCS_IPv4_transmit( stream_t* inputStream, const DPAUCS_IPv4_address_t* src, const DPAUCS_IPv4_address_t* dst, uint8_t type ){
  const uint16_t hl = 5;
  static uint16_t id = 0;

  uint8_t offset = 0;

  while(!DPAUCS_stream_eof(inputStream)){

    // prepare next packet
    DPAUCS_packet_info p;
    memset( &p, 0, sizeof(DPAUCS_packet_info) );
    p.type = ETH_TYPE_IP_V4;
    memcpy( p.destination_mac, dst->address.mac, 6 );
    memcpy( p.source_mac, src->address.mac, 6 );
    DPAUCS_preparePacket( &p );

    // create ip header
    DPAUCS_IPv4_t* ip = p.payload;
    ip->version_ihl = (uint16_t)( 4u << 4u ) | ( hl & 0x0F );
    ip->id = htob16( id );
    ip->ttl = DEFAULT_TTL;
    ip->protocol = type;
    ip->source = htob32( src->ip );
    ip->destination = htob32( dst->ip );

    // create content
    size_t s = DPAUCS_stream_read( inputStream, ((unsigned char*)p.payload) + hl * 4, (PACKET_MAX_PAYLOAD - sizeof(DPAUCS_IPv4_t)) & ~7u );

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
