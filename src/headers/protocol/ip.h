#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <packet.h>
#include <packed.h>

enum DPAUCS_ipv4_flags {
  IPv4_FLAG_DONT_FRAGMENT  = 0x02,
  IPv4_FLAG_MORE_FRAGMENTS = 0x04
};

typedef PACKED1 struct PACKED2 {
  unsigned version : 4;
  unsigned ihl : 4; // IP Header Length
  uint8_t tos; // Type of Service
  uint16_t length; // Total Length
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

typedef PACKED1 struct PACKED2 {
  unsigned version : 4;
  unsigned tafficClass1 : 4;
  unsigned tafficClass2 : 4;
  unsigned flowLabel1 : 4;
  uint16_t flowLabel2;
  uint16_t payloadLength;
  uint8_t nextHeader;
  uint8_t hopLimit;
  uint8_t sourceAddress[128];
  uint8_t destinationAddress[128];
} DPAUCS_ipv6_t;

typedef PACKED1 union PACKED2 {
  uint8_t version;
  DPAUCS_ipv4_t ipv4;
  DPAUCS_ipv6_t ipv6;
} DPAUCS_ip_t;

void DPAUCS_ip_handler( DPAUCS_packet_info* info );

#endif

