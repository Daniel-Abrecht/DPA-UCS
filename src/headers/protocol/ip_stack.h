#ifndef IP_STACK_H
#define IP_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <protocol/tcp_ip_stack_memory.h>

#define DEFAULT_TTL 64
#define DPAUCS_MAX_INCOMPLETE_IP_PACKETS 32

typedef struct {
  uint32_t srcIp; // largest fields first to reduce padding
  uint32_t destIp;
  uint8_t srcMac[6];
  // destMac: always the same as global variable mac, only provided to simplify future changes
  uint8_t destMac[6];
  uint16_t id;
  uint8_t tos; // Type of Service
  uint16_t offset;
  bool valid;
} DPAUCS_ipPacketInfo_t;

typedef struct { 
  DPAUCS_fragment_t fragment_info; // must be the first member
  DPAUCS_ipPacketInfo_t* info;
  uint16_t offset;
  uint16_t length;
} DPAUCS_ip_fragment_t;

DPAUCS_ip_fragment_t** DPAUCS_allocIpFragment( DPAUCS_ipPacketInfo_t*, uint16_t );
void DPAUCS_updateIpPackatOffset( DPAUCS_ip_fragment_t* );
void DPAUCS_removeIpPacket( DPAUCS_ipPacketInfo_t* );
bool DPAUCS_isNextIpFragment( DPAUCS_ip_fragment_t* );
bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ipPacketInfo_t*, DPAUCS_ipPacketInfo_t* );
DPAUCS_ip_fragment_t** DPAUCS_searchFollowingIpFragment( DPAUCS_ip_fragment_t* );
DPAUCS_ipPacketInfo_t* DPAUCS_normalize_ip_packet_info_ptr(DPAUCS_ipPacketInfo_t*);
DPAUCS_ipPacketInfo_t* DPAUCS_save_ip_packet_info(DPAUCS_ipPacketInfo_t* packet);

#endif
