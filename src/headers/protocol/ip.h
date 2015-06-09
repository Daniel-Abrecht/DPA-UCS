#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <packet.h>
#include <packed.h>
#include <stream.h>
#include <protocol/address.h>

#define MAX_IP_PROTO_HANDLERS 3


enum DPAUCS_ipv4_flags {
  IPv4_FLAG_DONT_FRAGMENT  = 0x02,
  IPv4_FLAG_MORE_FRAGMENTS = 0x04
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
} DPAUCS_ipv4_t;

typedef PACKED1 struct PACKED2 {
  uint8_t version_tafficClass1;
  uint8_t tafficClass2_flowLabel1;
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

typedef stream_t*(*DPAUCS_beginTransmission)( DPAUCS_address_t* from, DPAUCS_address_t** to, uint8_t type );
typedef void(*DPAUCS_endTransmission)();
typedef bool(*DPAUCS_ipProtocolReciveHandler_func)( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_beginTransmission beginTransmission, DPAUCS_endTransmission, uint16_t, uint16_t, void*, bool );
typedef void(*DPAUCS_ipProtocolFailtureHandler_func)( void* id );

typedef struct {
  uint8_t protocol;
  DPAUCS_ipProtocolReciveHandler_func onrecive;
  DPAUCS_ipProtocolFailtureHandler_func onrecivefailture;
} DPAUCS_ipProtocolHandler_t;

void DPAUCS_addIpProtocolHandler( DPAUCS_ipProtocolHandler_t* handler );
void DPAUCS_removeIpProtocolHandler( DPAUCS_ipProtocolHandler_t* handler );

#endif
