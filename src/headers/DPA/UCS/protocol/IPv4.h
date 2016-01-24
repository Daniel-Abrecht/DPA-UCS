#if !defined( IPv4_H ) && defined( USE_IPv4 )
#define IPv4_H

#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/ip_stack.h>

struct DPAUCS_packet_info;
struct DPAUCS_stream;

enum DPAUCS_IPv4_flags {
  IPv4_FLAG_MORE_FRAGMENTS = 1<<0,
  IPv4_FLAG_DONT_FRAGMENT  = 1<<1
};

typedef PACKED1 struct PACKED2 DPAUCS_IPv4 {
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

typedef struct DPAUCS_IPv4_address {
  DPAUCS_address_t address;
  uint32_t ip;
} DPAUCS_IPv4_address_t;

typedef struct DPAUCS_logicAddress_IPv4 {
  union {
    enum DPAUCS_address_types type;
    DPAUCS_logicAddress_t logicAddress;
  };
  uint32_t address;
} DPAUCS_logicAddress_IPv4_t;

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

void DPAUCS_IPv4_transmit(
  struct DPAUCS_stream* inputStream,
  const struct DPAUCS_IPv4_address* src,
  const struct DPAUCS_IPv4_address* dst,
  uint8_t type,
  size_t max_size
);

void DPAUCS_IPv4_handler(
  struct DPAUCS_packet_info* info,
  DPAUCS_IPv4_t* ip
);

#endif