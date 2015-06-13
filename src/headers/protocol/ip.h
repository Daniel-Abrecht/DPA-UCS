#ifndef IP_H
#define IP_H

#include <protocol/IPv4.h>
#include <protocol/IPv6.h>

#define MAX_IP_PROTO_HANDLERS 3

typedef PACKED1 union PACKED2 {
  uint8_t version;
  DPAUCS_IPv4_t IPv4;
  DPAUCS_IPv6_t ipv6;
} DPAUCS_ip_t;

void DPAUCS_ip_handler( DPAUCS_packet_info* info );

#endif
