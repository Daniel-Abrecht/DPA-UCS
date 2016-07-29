#ifndef DPAUCS_IP_STACK_H
#define DPAUCS_IP_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/tcp_ip_stack_memory.h>

#define DEFAULT_TTL 64
#define DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS 32

typedef struct DPAUCS_ip_packetInfo {
  enum DPAUCS_fragmentType type; // must be the first member
  bool valid;
  void(*onremove)(void*);
  uint16_t offset;
} DPAUCS_ip_packetInfo_t;

typedef struct DPAUCS_ip_fragment {
  DPAUCS_fragment_t fragment; // must be the first member
  struct DPAUCS_ip_packetInfo* info;
  uint16_t offset;
  uint16_t length;
  void* datas;
} DPAUCS_ip_fragment_t;

DPAUCS_ip_fragment_t** DPAUCS_layer3_allocFragment( DPAUCS_ip_packetInfo_t*, uint16_t );
void DPAUCS_layer3_updatePackatOffset( DPAUCS_ip_fragment_t* );
void DPAUCS_layer3_removePacket( DPAUCS_ip_packetInfo_t* );
bool DPAUCS_layer3_isNextFragment( DPAUCS_ip_fragment_t* );
bool DPAUCS_layer3_areFragmentsFromSamePacket( DPAUCS_ip_packetInfo_t*, DPAUCS_ip_packetInfo_t* );
DPAUCS_ip_fragment_t** DPAUCS_layer3_searchFollowingFragment( DPAUCS_ip_fragment_t* );
DPAUCS_ip_packetInfo_t* DPAUCS_layer3_normalize_packet_info_ptr(DPAUCS_ip_packetInfo_t*);
DPAUCS_ip_packetInfo_t* DPAUCS_layer3_save_packet_info(DPAUCS_ip_packetInfo_t* packet);
void DPAUCS_layer3_removeFragment( DPAUCS_ip_fragment_t** f );

#endif
