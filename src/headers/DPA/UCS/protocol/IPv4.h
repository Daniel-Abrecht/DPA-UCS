#ifndef DPAUCS_IPv4_H
#define DPAUCS_IPv4_H

#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/ip_stack.h>

#define DPAUCS_ETH_T_IPv4 0x0800

#define DPAUCS_LA_IPv4_INIT \
  .type = DPAUCS_ETH_T_IPv4

#define DPAUCS_LA_IPv4( a,b,c,d ) { \
    DPAUCS_LA_IPv4_INIT, \
    .ip = ((uint32_t)a<<24) \
        | ((uint32_t)b<<16) \
        | ((uint32_t)c<<8) \
        | ((uint32_t)d) \
  }

struct DPAUCS_packet_info;
struct DPA_stream;

enum DPAUCS_IPv4_flags {
  IPv4_FLAG_MORE_FRAGMENTS = 1<<0,
  IPv4_FLAG_DONT_FRAGMENT  = 1<<1
};

typedef struct packed DPAUCS_IPv4 {
  uint8_t version_ihl; // 4bit version | 4bit IP Header Length
  uint8_t tos; // Type of Service
  uint16_t length; // Total Length
  uint16_t id; // Identification
  uint8_t flags_offset1; // 3bit flags | 5bit Frame offset
  uint8_t offset2; // 8bit fragment offset
  uint8_t ttl; // Time to live
  // Protocol: http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
  uint8_t protocol;
  uint16_t checksum;
  uint32_t source; // source IP address
  uint32_t destination; // destination IP address
} DPAUCS_IPv4_t;

typedef struct DPAUCS_logicAddress_IPv4 {
  union {
    uint16_t type;
    DPAUCS_logicAddress_t logic;
  };
  uint32_t ip;
} DPAUCS_logicAddress_IPv4_t;

typedef struct DPAUCS_IPv4_address {
  DPAUCS_address_t address;
  char rawAddrMem[4]; // ensure enough space for IPv4 address
} DPAUCS_IPv4_address_t;

typedef struct DPAUCS_IPv4_packetInfo {
  DPAUCS_ip_packetInfo_t ipPacketInfo;
  DPAUCS_IPv4_address_t src;
  DPAUCS_IPv4_address_t dest;
  uint16_t id;
  uint8_t tos; // Type of Service
} DPAUCS_IPv4_packetInfo_t;

typedef struct DPAUCS_IPv4_fragment {
  DPAUCS_ip_fragment_t ipFragment;
  uint8_t flags;
} DPAUCS_IPv4_fragment_t;

bool DPAUCS_areFragmentsFromSameIPv4Packet(
  DPAUCS_IPv4_packetInfo_t*,
  DPAUCS_IPv4_packetInfo_t*
);

#endif
