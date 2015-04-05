#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <packet.h>
#include <packed.h>

typedef PACKED1 struct PACKED2 {
  unsigned version : 4;
  unsigned ihl : 4; // IP Header Length
  uint8_t tos; // Type of Service
  uint16_t id; // Identification
  unsigned flags : 3;
  unsigned offset_1 : 5; // Frame offset, splittet to avoid endiannes isuses
  uint8_t offset_2;
  uint8_t ttl; // Time to live
  // Protocol: http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
  uint8_t protocol;
  uint16_t checksum;
  uint32_t source; // source IP address
  uint32_t destination; // destination IP address
} DPAUCS_ipv4_t;

void DPAUCS_ipv4_handler( DPAUCS_packet_info* info );

#endif

