#ifndef DPAUCS_UDP_H
#define DPAUCS_UDP_H

#define PROTOCOL_UDP 17

#include <DPA/UCS/protocol/address.h>
#include <DPA/utils/helper_macros.h>

DPA_MODULE( udp );

typedef struct DPAUCS_udp_descriptor {
  DPAUCS_mixedAddress_pair_t fromTo;
  uint16_t source, destination;
} DPAUCS_udp_descriptor_t;

void DPAUCS_udp_make_reply_descriptor( DPAUCS_udp_descriptor_t* request, DPAUCS_udp_descriptor_t* reply );

#endif
