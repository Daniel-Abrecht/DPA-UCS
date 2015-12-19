#include <DPA/UCS/checksum.h>
#include <DPA/UCS/protocol/icmp.h>
#include <stdbool.h>
#include <DPA/UCS/protocol/layer3.h>

DPAUCS_MODUL( icmp ){}

typedef PACKED1 struct PACKED2 {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint8_t RoH[4];
} DPAUCS_icmp_t;

enum icmp_types {
  ICMP_ECHO_REPLY,
  ICMP_DESTINATION_UNREACHABLE = 3,
  ICMP_SOURCE_QUENCH,
  ICMP_REDIRECT,
  ICMP_ECHO_REQUEST = 8,
  ICMP_ROUTER_ADVERTISEMENT,
  ICMP_ROUTER_SOLICITATION,
  ICMP_TIME_EXCEEDED,
  ICMP_PARAMETER_PROBLEM,
  ICMP_TIMESTAMP,
  ICMP_TIMESTAMP_REPLY,
  ICMP_INFORMATION_REQUEST,
  ICMP_INFORMATION_REPLY,
  ICMP_ADDRESS_MASK_REQUEST,
  ICMP_ADDRESS_MASK_REPLY,
  ICMP_TRACEROUTE = 30,
  ICMP_DATAGRAM_CONVERSION_ERROR,
  ICMP_MOBILE_HOST_REDIRECT,
  ICMP_IPV6_WHERE_ARE_YOU,
  ICMP_IPV6_I_AM_HERE,
  ICMP_MOBILE_REGISTRATION_REQUEST,
  ICMP_MOBILE_REGISTRATION_REPLY,
  ICMP_DOMAIN_NAME_REQUEST,
  ICMP_DOMAIN_NAME_REPLY,
  ICMP_SKIP,
  ICMP_PHOTURIS
};

static bool icmp_reciveHandler( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, uint16_t offset, uint16_t length, DPAUCS_fragment_t** fragment, void* payload, bool last ){
  if(!last)
    return false;
  (void)offset;
  (void)length;
  (void)fragment;
  (void)id;
  DPAUCS_icmp_t* icmp = payload;
  switch( icmp->type ){
    case ICMP_ECHO_REQUEST: {
      icmp->type = ICMP_ECHO_REPLY;
      icmp->checksum = 0;
      icmp->checksum = checksum( payload, sizeof(DPAUCS_icmp_t) );
      DPAUCS_address_pair_t fromTo = {
        .source = to,
        .destination = from
      };
      DPAUCS_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
      DPAUCS_stream_referenceWrite( stream, payload, length );
      DPAUCS_layer3_transmit( stream, &fromTo, PROTOCOL_ICMP, ~0 );
      DPAUCS_layer3_destroyTransmissionStream( stream );
    } break;
  }
  return true;
}

static DPAUCS_layer3_protocolHandler_t icmp_handler = {
  .protocol = PROTOCOL_ICMP,
  .onreceive = &icmp_reciveHandler
};

static int counter = 0;

void DPAUCS_icmpInit(){
  if(counter++) return;
  DPAUCS_layer3_addProtocolHandler(&icmp_handler);
}

void DPAUCS_icmpShutdown(){
  if(--counter) return;
  DPAUCS_layer3_removeProtocolHandler(&icmp_handler);
}