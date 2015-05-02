#ifndef IP_STACK_H
#define IP_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <protocol/tcp_ip_stack_memory.h>

#define DEFAULT_TTL 64
#define DPAUCS_MAX_INCOMPLETE_IP_PACKETS 32
#define DPAUCS_IP_FRAGMENT_DATA_OFFSET ( offsetof(DPAUCS_ip_fragment_t,info) + sizeof(DPAUCS_ipPacketInfo_t*) )

typedef struct {
  uint32_t ip;
  uint8_t mac[6];
} eth_ip_address_t;

typedef struct DPAUCS_ipPacketInfo {
  eth_ip_address_t src;
  eth_ip_address_t dest;
  uint16_t id;
  uint8_t tos; // Type of Service
  uint16_t offset;
  bool valid;
  void(*onremove)(void*);
} DPAUCS_ipPacketInfo_t;

typedef struct { 
  // Header //
  DPAUCS_fragment_t fragment_info; // must be the first member
  DPAUCS_ipPacketInfo_t* info; // must be the second member
  // Datas //
  uint16_t offset;
  uint16_t length;
  uint8_t flags;
} DPAUCS_ip_fragment_t;

DPAUCS_ip_fragment_t** DPAUCS_allocIpFragment( DPAUCS_ipPacketInfo_t*, uint16_t );
void DPAUCS_updateIpPackatOffset( DPAUCS_ip_fragment_t* );
void DPAUCS_removeIpPacket( DPAUCS_ipPacketInfo_t* );
bool DPAUCS_isNextIpFragment( DPAUCS_ip_fragment_t* );
bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ipPacketInfo_t*, DPAUCS_ipPacketInfo_t* );
DPAUCS_ip_fragment_t** DPAUCS_searchFollowingIpFragment( DPAUCS_ip_fragment_t* );
DPAUCS_ipPacketInfo_t* DPAUCS_normalize_ip_packet_info_ptr(DPAUCS_ipPacketInfo_t*);
DPAUCS_ipPacketInfo_t* DPAUCS_save_ip_packet_info(DPAUCS_ipPacketInfo_t* packet);
void DPAUCS_removeIpFragment( DPAUCS_ip_fragment_t** f );

#endif
