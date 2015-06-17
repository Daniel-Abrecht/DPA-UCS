#ifndef ICMP_H
#define ICMP_H

#define PROTOCOL_ICMP 1

#include <stdbool.h>
#include <packed.h>
#include <protocol/layer3.h>

void DPAUCS_icmpInit();
void DPAUCS_icmpShutdown();

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

#endif
