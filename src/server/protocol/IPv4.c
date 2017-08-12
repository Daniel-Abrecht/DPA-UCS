#include <string.h>
#include <DPA/utils/utils.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/checksum.h>
#include <DPA/utils/logger.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>
#include <DPA/UCS/protocol/ip_stack.h>

DPA_MODULE( IPv4 ){}

struct DPAUCS_packet_info;
struct DPA_stream;

enum DPAUCS_IPv4_flags {
  IPv4_FLAG_MORE_FRAGMENTS = 1<<0,
  IPv4_FLAG_DONT_FRAGMENT  = 1<<1
};

typedef struct packed DPAUCS_IPv4 {
  uint8_t version_ihl; // 4bit version | 4bit IP Header Length
  uint8_t tos; // Type of Service
  uint16_t length; // Total Length
  uint16_t id; // Identification
  uint8_t flags_offset1; // 3bit flags | 5bit Frame offset
  uint8_t offset2; // 8bit fragment offset
  uint8_t ttl; // Time to live
  // Protocol: http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
  uint8_t protocol;
  uint16_t checksum;
  unsigned char source[4]; // source IP address
  unsigned char destination[4]; // destination IP address
} DPAUCS_IPv4_t;

typedef struct DPAUCS_IPv4_address {
  DPAUCS_address_t address;
  char rawAddrMem[4]; // ensure enough space for IPv4 address
} DPAUCS_IPv4_address_t;

typedef struct DPAUCS_IPv4_packetInfo {
  DPAUCS_ip_packetInfo_t ipPacketInfo;
  DPAUCS_IPv4_address_t src;
  DPAUCS_IPv4_address_t dest;
  uint16_t id;
  uint8_t tos; // Type of Service
} DPAUCS_IPv4_packetInfo_t;

typedef struct DPAUCS_IPv4_fragment {
  DPAUCS_ip_fragment_t ipFragment;
  uint8_t flags;
} DPAUCS_IPv4_fragment_t;


static DPAUCS_fragmentHandler_t fragment_handler;

static void packetHandler( DPAUCS_packet_info_t* info ){
  if( (*(uint8_t*)info->payload) >> 4 != 4 ) // Version must be 4
    return;

  DPAUCS_IPv4_t* ip = info->payload;
  if( DPA_btoh16(ip->length) > info->size )
    return;

  char source[4];
  char destination[4];

  memcpy(source,ip->source,4);
  memcpy(destination,ip->destination,4);

  {
    DPAUCS_logicAddress_t* addr = DPAUCS_LA_IPv4(0,0,0,0);
    memcpy(DPAUCS_GET_ADDR(addr),destination,4);
    if( !DPAUCS_isValidHostAddress(addr)
     || !DPAUCS_has_logicAddress(addr)
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
      .handler = &fragment_handler,
      .valid = true,
      .onremove = handler->onreceivefailture,
      .offset = 0
    },
    .src = {
      .address = {
        .type = DPAUCS_ETH_T_IPv4
      },
    },
    .dest = {
      .address = {
        .type = DPAUCS_ETH_T_IPv4
      },
    },
    .id = ip->id,
    .tos = ip->tos,
  };

  memcpy( DPAUCS_GET_ADDR( &ipInfo.src.address ), source, 4 );
  memcpy( DPAUCS_GET_ADDR( &ipInfo.dest.address ), destination, 4 );
  memcpy( ipInfo.src.address.mac,info->source_mac, sizeof(DPAUCS_mac_t) );
  memcpy( ipInfo.dest.address.mac,info->destination_mac, sizeof(DPAUCS_mac_t) );

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
  size_t header_count,
  const size_t header_size[header_count],
  const void* header_data[header_count],
  DPA_stream_t* inputStream,
  const DPAUCS_mixedAddress_pair_t* fromTo,
  uint8_t type,
  size_t max_size_arg
){

  const DPAUCS_address_t* src = DPAUCS_mixedPairComponentToAddress( fromTo, true );
  const DPAUCS_address_t* dst = DPAUCS_mixedPairComponentToAddress( fromTo, false );
  DPAUCS_address_t* tmp_IPv4Addr = DPAUCS_ADDR_IPv4;

  static const uint16_t hl = 5;
  static uint16_t id = 0;

  if(!dst){
    DPA_LOG( "transmit: destination address incomplete\n" );
    return false;
  }
  const DPAUCS_logicAddress_t* laddr = src ? &src->logic
                                           : DPAUCS_mixedPairComponentToLogicAddress( fromTo, true );
  if(!laddr){
    DPA_LOG("transmit: Can't convert source address\n");
    return false;
  }
  const DPAUCS_interface_t* interface = DPAUCS_getInterface( laddr );
  if( !interface ){
    DPA_LOG( "transmit: Can't find source interface\n" );
    return false;
  }
  if(!src){
    DPAUCS_copy_logicAddress( &tmp_IPv4Addr->logic, laddr );
    memcpy( tmp_IPv4Addr->mac, interface->mac, sizeof(DPAUCS_mac_t) );
    src = tmp_IPv4Addr;
  }

  for( size_t i=0; i<header_count; i++ ){
    if( max_size_arg + header_size[i] < max_size_arg ){
      max_size_arg = ~0;
    }else{
      max_size_arg += header_size[i];
    }
  }
  uint16_t max_size = DPA_MIN(max_size_arg,(uint16_t)~0);

  size_t offset = 0, header = 0;

  while( offset < max_size && ( header < header_count || ( inputStream && !DPA_stream_eof(inputStream) ) ) ){

    // prepare next packet
    DPAUCS_packet_info_t p;
    memset( &p, 0, sizeof(DPAUCS_packet_info_t) );
    p.type = DPAUCS_ETH_T_IPv4;
    p.interface = interface;
    memcpy( p.destination_mac, dst->mac, sizeof(DPAUCS_mac_t) );
    memcpy( p.source_mac, src->mac, sizeof(DPAUCS_mac_t) );
    DPAUCS_preparePacket( &p );

    // create ip header
    DPAUCS_IPv4_t* ip = p.payload;
    ip->version_ihl = (uint16_t)( 4u << 4u ) | ( hl & 0x0F );
    ip->id = DPA_htob16( id );
    ip->ttl = DEFAULT_TTL;
    ip->protocol = type;
    memcpy( ip->source, DPAUCS_GET_ADDR(src), 4 );
    memcpy( ip->destination, DPAUCS_GET_ADDR(dst), 4 );

    // create content
    size_t msize = DPA_MIN( (uint16_t)max_size - offset, (uint16_t)PACKET_MAX_PAYLOAD - sizeof(DPAUCS_IPv4_t) );
    size_t s = 0;
    {
      unsigned char* data = ((unsigned char*)p.payload) + hl * 4;
      while( s < msize ){
        if( header < header_count ){
          size_t hs = header_size[header];
          if( hs > msize - s )
            hs = msize - s;
          memcpy( data, header_data[header], hs );
          data += hs;
          s += hs;
          header++;
        }else if(inputStream){
          s += DPA_stream_read( inputStream, data, msize );
          break;
        }else break;
      }
    }

    // complete ip header
    uint8_t flags = 0;

    if( ( header < header_count || ( inputStream && !DPA_stream_eof(inputStream) ) ) && (uint32_t)(offset+s) < max_size )
      flags |= IPv4_FLAG_MORE_FRAGMENTS;

    ip->length = DPA_htob16( p.size + hl * 4 + s );
    ip->flags_offset1 = ( flags << 5 ) | ( ( offset >> 11 ) & 0x1F );
    ip->offset2 = ( offset >> 3 ) & 0xFF;

    ip->checksum = 0;
    ip->checksum = checksum( p.payload, hl * 4 );

    // send packet
    DPAUCS_sendPacket( &p, hl * 4 + s );
    DPA_LOG(
      "transmit: Sent fragment offset %zu, payload size %zu\n",
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

static bool isBroadcast( const DPAUCS_logicAddress_t* address ){
  return !memcmp(DPAUCS_GET_ADDR(address),"\xFF\xFF\xFF\xFF",4);
}

static bool isValid( const DPAUCS_logicAddress_t* address ){
  return memcmp(DPAUCS_GET_ADDR(address),"\x0\x0\x0",4)
      && memcmp(DPAUCS_GET_ADDR(address),"\xFF\xFF\xFF\xFF",4);
}

static bool compare( const DPAUCS_logicAddress_t* a, const DPAUCS_logicAddress_t* b ){
  return !memcmp(DPAUCS_GET_ADDR(a),DPAUCS_GET_ADDR(b),4);
}

static bool copy( DPAUCS_logicAddress_t* dst, const DPAUCS_logicAddress_t* src ){
  memcpy(DPAUCS_GET_ADDR(dst),DPAUCS_GET_ADDR(src),4);
  return true;
}

static bool withRawAsLogicAddress( void* addr, size_t size, void(*func)(const DPAUCS_logicAddress_t*,void*), void* param ){
  if( size != 4 )
    return false;
  DPAUCS_logicAddress_t* laddr = DPAUCS_LA_IPv4(0,0,0,0);
  memcpy( DPAUCS_GET_ADDR( laddr ), addr, 4 );
  (*func)( laddr, param );
  return true;
}

static uint16_t calculatePseudoHeaderChecksum(
  const DPAUCS_logicAddress_t* source,
  const DPAUCS_logicAddress_t* destination,
  uint8_t protocol,
  uint16_t length
){
  struct packed pseudoHeader {
    unsigned char src[4], dst[4];
    uint8_t padding, protocol;
    uint16_t length;
  };
  struct pseudoHeader pseudoHeader = {
    .padding = 0,
    .protocol = protocol,
    .length = DPA_htob16( length )
  };
  memcpy( pseudoHeader.src, DPAUCS_GET_ADDR(source), 4 );
  memcpy( pseudoHeader.dst, DPAUCS_GET_ADDR(destination), 4 );
  return checksum( &pseudoHeader, sizeof(pseudoHeader) );
}


static bool isSamePacket( DPAUCS_ip_packetInfo_t* ta, DPAUCS_ip_packetInfo_t* tb ){
  DPAUCS_IPv4_packetInfo_t* a = (DPAUCS_IPv4_packetInfo_t*)ta;
  DPAUCS_IPv4_packetInfo_t* b = (DPAUCS_IPv4_packetInfo_t*)tb;
  return (
      a->id == b->id
   && a->tos == b->tos
   && !memcmp( DPAUCS_GET_ADDR(&a->src.address), DPAUCS_GET_ADDR(&b->src.address), 4 )
   && !memcmp( DPAUCS_GET_ADDR(&a->dest.address), DPAUCS_GET_ADDR(&b->dest.address), 4 )
   && !memcmp( a->src.address.mac, b->src.address.mac, sizeof(DPAUCS_mac_t) )
   && !memcmp( a->dest.address.mac, b->dest.address.mac, sizeof(DPAUCS_mac_t) )
  );
}

static void fragmentDestructor(DPAUCS_fragment_t** f){
  DPAUCS_layer3_removePacket(((DPAUCS_ip_fragment_t*)*f)->info);
}

static bool fragmentBeforeTakeover( DPAUCS_fragment_t*** f, DPAUCS_fragmentHandler_t* newHandler ){
  DPAUCS_ip_fragment_t** ipf = (DPAUCS_ip_fragment_t**)*f;
  DPAUCS_ip_packetInfo_t* ipfi = (*ipf)->info;
  if( !ipfi->valid )
    return false;
  if( (*ipf)->datas ){
    DPAUCS_fragment_t** result = DPAUCS_createFragment( newHandler, (*ipf)->length );
    if(!result)
      return false;
    *f = result;
    memcpy( DPAUCS_getFragmentData( *result ), (*ipf)->datas,(*ipf)->length );
  }
  ipfi->valid = false;
  return true;
}

static void takeoverFailtureHandler(DPAUCS_fragment_t** f){
  ((DPAUCS_ip_fragment_t*)*f)->info->valid = true;
  DPAUCS_layer3_removeFragment((DPAUCS_ip_fragment_t**)f);
}

static DPAUCS_fragmentHandler_t fragment_handler = {
  .packetInfo_size = sizeof(DPAUCS_IPv4_packetInfo_t),
  .fragmentInfo_size = sizeof(DPAUCS_IPv4_fragment_t),
  .isSamePacket = &isSamePacket,
  .destructor = &fragmentDestructor,
  .beforeTakeover = &fragmentBeforeTakeover,
  .takeoverFailtureHandler = &takeoverFailtureHandler
};

static const DPAUCS_l3_handler_t l3_handler = {
  .type = DPAUCS_ETH_T_IPv4,
  .rawAddressSize = 4,
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


DPAUCS_EXPORT_FRAGMENT_HANDLER( IPv4, &fragment_handler );
DPAUCS_EXPORT_L3_HANDLER( IPv4, &l3_handler );
