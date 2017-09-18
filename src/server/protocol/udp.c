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
      ch += ~(*handler->calculatePseudoHeaderChecksum)( &from->logic, &to->logic, PROTOCOL_UDP, length );
    ch = (uint16_t)~( ( ch & 0xFFFF ) + ( ch >> 16 ) );
    if( ch )
      return false;
  }
  realLength -= sizeof(DPAUCS_udp_t);
  const flash DPAUCS_service_t* service = DPAUCS_get_service( &to->logic, udp_descriptor.destination, PROTOCOL_UDP );
  if(!service)
    return false;
  if( service->onopen )
    (*service->onopen)( &udp_descriptor );
  if(service->onreceive)
    (*service->onreceive)( &udp_descriptor, udp + 1, realLength );
  if( service->onclose )
    (*service->onclose)( &udp_descriptor );
  return true;
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
