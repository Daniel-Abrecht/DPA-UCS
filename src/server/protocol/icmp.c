#include <checksum.h>
#include <protocol/icmp.h>

static bool icmp_reciveHandler( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_beginTransmission begin, DPAUCS_endTransmission end, uint16_t offset, uint16_t length, void* payload, bool last ){
  if(!last)
    return false;
  (void)offset;
  (void)length;
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
      stream_t* stream = (*begin)( &fromTo, 1, IP_PROTOCOL_ICMP );
      DPAUCS_stream_referenceWrite( stream, payload, length );
      (*end)();
    } break;
  }
  return true;
}

static DPAUCS_layer3_protocolHandler_t icmp_handler = {
  .protocol = IP_PROTOCOL_ICMP,
  .onrecive = &icmp_reciveHandler
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