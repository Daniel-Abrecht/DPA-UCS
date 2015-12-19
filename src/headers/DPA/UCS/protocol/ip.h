#ifndef IP_H
#define IP_H

#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/IPv6.h>

#define MAX_IP_PROTO_HANDLERS 3

typedef PACKED1 union PACKED2 {
  uint8_t version;
#ifdef USE_IPv4
  DPAUCS_IPv4_t IPv4;
#endif
#ifdef USE_IPv6
  DPAUCS_IPv6_t IPv6;
#endif
} DPAUCS_ip_t;

void DPAUCS_ip_handler( DPAUCS_packet_info* info );

#endif
