#include <string.h>
#include <DPA/utils/utils.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/checksum.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>

static void packetHandler( DPAUCS_packet_info_t* info ){
  if( (*(uint8_t*)info->payload) >> 4 != 4 ) // Version must be 4
    return;

  DPAUCS_IPv4_t* ip = info->payload;
  if( DPA_btoh16(ip->length) > info->size )
    return;

  uint32_t source = DPA_btoh32(ip->source);
  uint32_t destination = DPA_btoh32(ip->destination);

  {
    DPAUCS_logicAddress_IPv4_t addr = {
      DPAUCS_LA_IPv4_INIT,
      .address = destination
    };
    if( !DPAUCS_isValidHostAddress(&addr.logicAddress)
     || !DPAUCS_has_logicAddress(&addr.logicAddress)
    ) return;
  }

  DPAUCS_l4_handler_t* handler = 0;
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( l4_handlers[i]
     && l4_handlers[i]->protocol == ip->protocol
    ){
      handler = l4_handlers[i];
      break;
    }
  if(!handler) return; 

  uint16_t headerlength = (ip->version_ihl & 0x0F) * 4;

  if( checksum( ip, headerlength ) )
    return; // invalid checksum

  DPAUCS_IPv4_packetInfo_t ipInfo = {
    .ipPacketInfo = {
      .type = DPAUCS_FRAGMENT_TYPE_IPv4,
      .valid = true,
      .onremove = handler->onreceivefailture,
      .offset = 0
    },
    .src = {
      .address = {
        .type = DPAUCS_ETH_T_IPv4
      },
      .ip = source
    },
    .dest = {
      .address = {
        .type = DPAUCS_ETH_T_IPv4
      },
      .ip = destination
    },
    .id = ip->id,
    .tos = ip->tos,
  };
  memcpy(ipInfo.src.address.mac,info->source_mac,sizeof(DPAUCS_mac_t));
  memcpy(ipInfo.dest.address.mac,info->destination_mac,sizeof(DPAUCS_mac_t));

  uint8_t* payload = ((uint8_t*)ip) + headerlength;

  DPAUCS_IPv4_fragment_t fragment = {
    .ipFragment = {
      .offset = (uint16_t)( (uint16_t)( ip->flags_offset1 & 0x1F ) | (uint16_t)ip->offset2 << 5u ) * 8u,
      .length = DPA_btoh16(ip->length) - headerlength,
      .datas = payload
    },
    .flags = ( ip->flags_offset1 >> 5 ) & 0x07
  };

  DPAUCS_IPv4_packetInfo_t* ipi = (DPAUCS_IPv4_packetInfo_t*)DPAUCS_layer3_normalize_packet_info_ptr(&ipInfo.ipPacketInfo);
  bool isNext;
  if(ipi){
    fragment.ipFragment.info = (DPAUCS_ip_packetInfo_t*)ipi;
    isNext = DPAUCS_layer3_isNextFragment(&fragment.ipFragment);
  }else{
    isNext = !fragment.ipFragment.offset; 
  }

  if( fragment.flags & IPv4_FLAG_MORE_FRAGMENTS || !isNext ){
    if(!ipi)
      ipi = (DPAUCS_IPv4_packetInfo_t*)DPAUCS_layer3_save_packet_info(&ipInfo.ipPacketInfo);
    fragment.ipFragment.info = (DPAUCS_ip_packetInfo_t*)ipi;
    if(isNext){
      DPAUCS_layer3_updatePackatOffset(&fragment.ipFragment);
      DPAUCS_IPv4_fragment_t* f = &fragment;
      DPAUCS_IPv4_fragment_t** f_ptr = &f;
      while(true){
        if(
            !(*handler->onreceive)(
              ipi,
              &((DPAUCS_IPv4_packetInfo_t*)f->ipFragment.info)->src.address,
              &((DPAUCS_IPv4_packetInfo_t*)f->ipFragment.info)->dest.address,
              f->ipFragment.offset,
              f->ipFragment.length,
              (DPAUCS_fragment_t**)f_ptr,
              payload,
              !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
            )
         || !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
        ){
          ipi->ipPacketInfo.onremove = 0;
          DPAUCS_layer3_removePacket(f->ipFragment.info);
        }else{
          DPAUCS_layer3_removeFragment((DPAUCS_ip_fragment_t**)f_ptr);
        }
        f_ptr = (DPAUCS_IPv4_fragment_t**)DPAUCS_layer3_searchFollowingFragment(&(*f_ptr)->ipFragment);
        if(!f_ptr)
          break;
        f = *f_ptr;
        payload = DPAUCS_getFragmentData( &f->ipFragment.fragment );
      }
    }else{
      DPAUCS_IPv4_fragment_t** f_ptr = (DPAUCS_IPv4_fragment_t**)DPAUCS_layer3_allocFragment(&ipi->ipPacketInfo,fragment.ipFragment.length);
      if(!f_ptr)
        return;
      (*f_ptr)->ipFragment.offset = fragment.ipFragment.offset;
      (*f_ptr)->ipFragment.length = fragment.ipFragment.length;
      (*f_ptr)->flags = fragment.flags;
      (*f_ptr)->ipFragment.datas = 0;
      memcpy( DPAUCS_getFragmentData( &(*f_ptr)->ipFragment.fragment ), payload, fragment.ipFragment.length );
      return;
    }
  }else{
    DPAUCS_IPv4_packetInfo_t* ipp = ipi ? ipi : &ipInfo;
    (*handler->onreceive)(
      ipp,
      &ipp->src.address,
      &ipp->dest.address,
      fragment.ipFragment.offset,
      fragment.ipFragment.length,
      (DPAUCS_fragment_t*[]){(DPAUCS_fragment_t*)&fragment},
      payload,
      !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
    );
    if(ipi){
      ipi->ipPacketInfo.onremove = 0;
      DPAUCS_layer3_removePacket(&ipi->ipPacketInfo);
    }
    ipi = 0;
  }

}

static bool transmit(
  DPA_stream_t* inputStream,
  const DPAUCS_mixedAddress_pair_t* fromTo,
  uint8_t type,
  size_t max_size_arg
){

  const DPAUCS_IPv4_address_t* src = (const DPAUCS_IPv4_address_t*)DPAUCS_mixedPairComponentToAddress( fromTo, true );
  const DPAUCS_IPv4_address_t* dst = (const DPAUCS_IPv4_address_t*)DPAUCS_mixedPairComponentToAddress( fromTo, false );
  DPAUCS_IPv4_address_t tmp_IPv4Addr = { DPAUCS_IPv4_INIT };

  static const uint16_t hl = 5;
  static uint16_t id = 0;

  if(!dst){
    DPA_LOG( "DPAUCS_IPv4_transmit: destination address incomplete\n" );
    return false;
  }
  const DPAUCS_logicAddress_t* laddr = src ? &src->address.logicAddress
                                           : DPAUCS_mixedPairComponentToLogicAddress( fromTo, true );
  if(!laddr){
    DPA_LOG("DPAUCS_IPv4_transmit: Can't convert source address\n");
    return false;
  }
  const DPAUCS_interface_t* interface = DPAUCS_getInterface( laddr );
  if( !interface ){
    DPA_LOG("DPAUCS_IPv4_transmit: Can't find source interface\n");
    return false;
  }
  if(!src){
    DPAUCS_copy_logicAddress( &tmp_IPv4Addr.address.logicAddress, laddr );
    memcpy( tmp_IPv4Addr.address.mac, interface->mac, sizeof(DPAUCS_mac_t) );
    src = &tmp_IPv4Addr;
  }

  uint16_t max_size = DPA_MIN(max_size_arg,(uint16_t)~0);

  size_t offset = 0;

  while( !DPA_stream_eof(inputStream) && offset < max_size ){

    // prepare next packet
    DPAUCS_packet_info_t p;
    memset( &p, 0, sizeof(DPAUCS_packet_info_t) );
    p.type = DPAUCS_ETH_T_IPv4;
    p.interface = interface;
    memcpy( p.destination_mac, dst->address.mac, sizeof(DPAUCS_mac_t) );
    memcpy( p.source_mac, src->address.mac, sizeof(DPAUCS_mac_t) );
    DPAUCS_preparePacket( &p );

    // create ip header
    DPAUCS_IPv4_t* ip = p.payload;
    ip->version_ihl = (uint16_t)( 4u << 4u ) | ( hl & 0x0F );
    ip->id = DPA_htob16( id );
    ip->ttl = DEFAULT_TTL;
    ip->protocol = type;
    ip->source = DPA_htob32( src->ip );
    ip->destination = DPA_htob32( dst->ip );

    // create content
    size_t msize = DPA_MIN( (uint16_t)max_size - offset, (uint16_t)PACKET_MAX_PAYLOAD - sizeof(DPAUCS_IPv4_t) );
    size_t s = DPA_stream_read( inputStream, ((unsigned char*)p.payload) + hl * 4, msize );

    // complete ip header
    uint8_t flags = 0;

    if( !DPA_stream_eof(inputStream) && (uint32_t)(offset+s) < max_size )
      flags |= IPv4_FLAG_MORE_FRAGMENTS;

    ip->length = DPA_htob16( p.size + hl * 4 + s );
    ip->flags_offset1 = ( flags << 5 ) | ( ( offset >> 11 ) & 0x1F );
    ip->offset2 = ( offset >> 3 ) & 0xFF;

    ip->checksum = 0;
    ip->checksum = checksum( p.payload, hl * 4 );

    // send packet
    DPAUCS_sendPacket( &p, hl * 4 + s );
    DPA_LOG(
      "IPv4: Sent fragment offset %lu, payload size %lu\n",
      offset, s
    );

    uint16_t offset_old = offset;
    // increment ip packet data offset
    offset += s;
    if( offset < offset_old )
      break;

  }

  id++;
  return true;
}

static bool isBroadcast( const DPAUCS_logicAddress_IPv4_t* address ){
  return address->address == 0xFFFFFFFF;
}

static bool isValid( const DPAUCS_logicAddress_IPv4_t* address ){
  return address->address && address->address != 0xFFFFFFFF;
}

static bool compare( const DPAUCS_logicAddress_IPv4_t* a, const DPAUCS_logicAddress_IPv4_t* b ){
  return a->address == b->address;
}

static bool copy( DPAUCS_logicAddress_IPv4_t* dst, const DPAUCS_logicAddress_IPv4_t* src ){
  *dst = *src;
  return true;
}

static bool withRawAsLogicAddress( uint16_t type, void* vaddr, size_t size, void(*func)(const DPAUCS_logicAddress_IPv4_t*,void*), void* param ){
  if( size != 4 )
    return false;
  uint32_t* addr = vaddr;
  DPAUCS_logicAddress_IPv4_t laddr = {
    .type = type,
    .address = DPA_btoh32(*addr)
  };
  (*func)( &laddr, param );
  return true;
}

static uint16_t calculatePseudoHeaderChecksum(
  const DPAUCS_logicAddress_IPv4_t* source,
  const DPAUCS_logicAddress_IPv4_t* destination,
  uint8_t protocol,
  uint16_t length
){
  struct packed pseudoHeader {
    uint32_t src, dst;
    uint8_t padding, protocol;
    uint16_t length;
  };
  struct pseudoHeader pseudoHeader = {
    .src = DPA_htob32( source->address ),
    .dst = DPA_htob32( destination->address ),
    .padding = 0,
    .protocol = protocol,
    .length = DPA_htob16( length )
  };
  return checksum( &pseudoHeader, sizeof(pseudoHeader) );
}

DEFINE_ADDRESS_HANDLER_TYPE( IPv4 );

static const DPAUCS_IPv4_l3_handler_t handler = {
  .type = DPAUCS_ETH_T_IPv4,
  .packetSizeLimit = 0xFFFFu,
  .packetHandler = &packetHandler,
  .transmit = &transmit,
  .isBroadcast = &isBroadcast,
  .isValid = &isValid,
  .compare = &compare,
  .copy = &copy,
  .withRawAsLogicAddress = &withRawAsLogicAddress,
  .calculatePseudoHeaderChecksum = &calculatePseudoHeaderChecksum
};

DPAUCS_EXPORT_ADDRESS_HANDLER( IPv4, &handler );
