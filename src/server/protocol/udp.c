#define DPAUCS_UDP_C

#include <stdbool.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/service.h>
#include <DPA/UCS/checksum.h>
#include <DPA/UCS/protocol/udp.h>
#include <DPA/UCS/protocol/layer3.h>

DPA_MODULE( udp ){}

typedef struct packed {
  uint16_t source, destination;
  uint16_t length;
  uint16_t checksum;
} DPAUCS_udp_t;

static bool udp_receiveHandler(
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t offset,
  uint16_t length,
  DPAUCS_fragment_t** fragment,
  void* payload,
  bool last
){
  if(!last)
    return false;
  if( length < sizeof(DPAUCS_udp_t) )
    return false;
  (void)offset;
  (void)length;
  (void)fragment;
  (void)id;
  DPAUCS_udp_t* udp = payload;
  uint16_t realLength = DPA_btoh16(udp->length);
  if( realLength > length || realLength < sizeof(DPAUCS_udp_t) )
    return false;
  DPAUCS_address_pair_t ap = {
    .destination = to,
    .source      = from
  };
  DPAUCS_udp_descriptor_t udp_descriptor = {
    .source = DPA_btoh16(udp->source),
    .destination = DPA_btoh16(udp->destination)
  };
  DPAUCS_addressPairToMixed( &udp_descriptor.fromTo, &ap );
  if( udp->checksum ){ // Check checksum if there is one
    uint32_t ch = (uint16_t)~checksum( payload, realLength );
    const flash DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( to->type );
    if( handler && handler->calculatePseudoHeaderChecksum )
      ch += ~(*handler->calculatePseudoHeaderChecksum)( &from->logic, &to->logic, PROTOCOL_UDP, realLength );
    ch = (uint16_t)~( ( ch & 0xFFFF ) + ( ch >> 16 ) );
    if( ch )
      return false;
  }
  const flash DPAUCS_service_t* service = DPAUCS_get_service( &to->logic, udp_descriptor.destination, PROTOCOL_UDP );
  if(!service)
    return false;
  const void* ssdata = DPAUCS_get_service_ssdata( &to->logic, udp_descriptor.destination, PROTOCOL_UDP );
  if( service->onopen )
    (*service->onopen)( &udp_descriptor, ssdata );
  if(service->onreceive)
    (*service->onreceive)( &udp_descriptor, udp + 1, realLength - sizeof(DPAUCS_udp_t) );
  if( service->onclose )
    (*service->onclose)( &udp_descriptor );
  return true;
}

void DPAUCS_udp_transmit( const DPAUCS_udp_descriptor_t* desc, const linked_data_list_t* next_header, DPA_stream_t* stream ){
  size_t size = DPA_stream_getLength( stream, ~0, 0 );
  DPAUCS_udp_t udp = {
    .source = DPA_htob16(desc->source),
    .destination = DPA_htob16(desc->destination),
    .length = 0,
    .checksum = 0
  };
  const linked_data_list_t* headers = (linked_data_list_t[]){{
    .size = sizeof(udp),
    .data = &udp,
    .next = next_header
  }};
  size_t header_size = 0;
  for( const linked_data_list_t* it = headers; it; it = it->next )
    header_size += it->size;
  udp.length = DPA_htob16( size + header_size );
  DPAUCS_logicAddress_pair_t fromTo = {0};
  DPAUCS_mixedPairToLogicAddress( &fromTo, &desc->fromTo );
  const flash DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( DPAUCS_mixedPairGetType( &desc->fromTo ) );
  uint32_t ch = 0;
  for( const linked_data_list_t* it = headers; it; it = it->next )
    ch += (uint16_t)~checksum( it->data, it->size );
  if( handler && handler->calculatePseudoHeaderChecksum )
    ch += (uint16_t)~(*handler->calculatePseudoHeaderChecksum)( fromTo.source, fromTo.destination, PROTOCOL_UDP, size + header_size );
  ch += (uint16_t)~checksumOfStream( stream, size );
  udp.checksum = ~( ( ch >> 16 ) + ( ch & 0xFFFF ) );
  DPAUCS_layer3_transmit( headers, stream, &desc->fromTo, PROTOCOL_UDP, ~0 );
}

void DPAUCS_udp_make_reply_descriptor( DPAUCS_udp_descriptor_t* request, DPAUCS_udp_descriptor_t* reply ){
  reply->source = request->destination;
  reply->destination = request->source;
  reply->fromTo = request->fromTo;
  DPAUCS_swapMixedAddress(&reply->fromTo);
}

static const flash DPAUCS_l4_handler_t udp_handler = {
  .protocol = PROTOCOL_UDP,
  .onreceive = &udp_receiveHandler
};

DPA_LOOSE_LIST_ADD( DPAUCS_l4_handler_list, &udp_handler )
