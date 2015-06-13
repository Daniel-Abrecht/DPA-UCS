#ifndef IP_STACK_H
#define IP_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <protocol/tcp_ip_stack_memory.h>
#include <protocol/address.h>
#include <protocol/IPv4.h>

#define DEFAULT_TTL 64
#define DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS 32

typedef struct {
  enum DPAUCS_fragmentType type; // must be the first member
  bool valid;
  void(*onremove)(void*);
  uint16_t offset;
} DPAUCS_ip_packetInfo_t;

typedef struct {
  DPAUCS_ip_packetInfo_t ipPacketInfo;
  DPAUCS_IPv4_address_t src;
  DPAUCS_IPv4_address_t dest;
  uint16_t id;
  uint8_t tos; // Type of Service
} DPAUCS_IPv4_packetInfo_t;

typedef struct {
  DPAUCS_fragment_t fragmentInfo; // must be the first member
  DPAUCS_ip_packetInfo_t* info;
  uint16_t offset;
  uint16_t length;
} DPAUCS_ip_fragment_t;

typedef struct {
  DPAUCS_ip_fragment_t ipFragment;
  uint8_t flags;
} DPAUCS_IPv4_fragment_t;

typedef union {
  DPAUCS_ip_packetInfo_t ip_packetInfo;
  DPAUCS_IPv4_packetInfo_t IPv4_packetInfo;
} DPAUCS_layer3_packetInfo_t;

DPAUCS_ip_fragment_t** DPAUCS_layer3_allocFragment( DPAUCS_ip_packetInfo_t*, uint16_t );
void DPAUCS_updateIpPackatOffset( DPAUCS_ip_fragment_t* );
void DPAUCS_removeIpPacket( DPAUCS_ip_packetInfo_t* );
bool DPAUCS_isNextIpFragment( DPAUCS_ip_fragment_t* );
bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ip_packetInfo_t*, DPAUCS_ip_packetInfo_t* );
DPAUCS_ip_fragment_t** DPAUCS_searchFollowingIpFragment( DPAUCS_ip_fragment_t* );
DPAUCS_ip_packetInfo_t* DPAUCS_normalize_ip_packet_info_ptr(DPAUCS_ip_packetInfo_t*);
DPAUCS_ip_packetInfo_t* DPAUCS_save_ip_packet_info(DPAUCS_ip_packetInfo_t* packet);
void DPAUCS_removeIpFragment( DPAUCS_ip_fragment_t** f );

#endif
