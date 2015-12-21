#if !defined( IPv4_H ) && defined( USE_IPv4 )
#define IPv4_H

#include <DPA/UCS/packet.h>
#include <DPA/UCS/stream.h>
#include <string.h>
#include <DPA/UCS/checksum.h>
#include <DPA/UCS/binaryUtils.h>
#include <DPA/UCS/protocol/address.h>

enum DPAUCS_IPv4_flags {
  IPv4_FLAG_MORE_FRAGMENTS = 1<<0,
  IPv4_FLAG_DONT_FRAGMENT  = 1<<1
};

typedef PACKED1 struct PACKED2 {
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

typedef struct {
  DPAUCS_address_t address;
  uint32_t ip;
} DPAUCS_IPv4_address_t;

typedef struct {
  union {
    DPAUCS_address_types_t type;
    DPAUCS_logicAddress_t logicAddress;
  };
  uint32_t address;
} DPAUCS_logicAddress_IPv4_t;

void DPAUCS_IPv4_transmit(
  DPAUCS_stream_t* inputStream,
  const DPAUCS_IPv4_address_t* src,
  const DPAUCS_IPv4_address_t* dst,
  uint8_t type,
  size_t max_size
);

void DPAUCS_IPv4_handler(
  DPAUCS_packet_info* info,
  DPAUCS_IPv4_t* ip
);

#endif