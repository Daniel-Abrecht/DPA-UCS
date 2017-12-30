#ifndef DPAUCS_IP_STACK_H
#define DPAUCS_IP_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/fragment_memory.h>
#include <DPA/utils/helper_macros.h>

#define DEFAULT_TTL 64
#ifndef DPA_MAX_PACKET_INFO_BUFFER_SIZE
#define DPA_MAX_PACKET_INFO_BUFFER_SIZE 512u
#endif

struct DPAUCS_fragmentHandler;
struct DPAUCS_ip_packetInfo;

DPA_LOOSE_LIST_DECLARE( const flash struct DPAUCS_fragmentHandler*, DPAUCS_fragmentHandler_list )

typedef struct DPAUCS_fragmentHandler {
  size_t packetInfo_size;
  size_t fragmentInfo_size;
  bool (*isSamePacket)( struct DPAUCS_ip_packetInfo*, struct DPAUCS_ip_packetInfo* );
  void (*destructor)( DPAUCS_fragment_t** );
  bool (*beforeTakeover)( DPAUCS_fragment_t***, const flash struct DPAUCS_fragmentHandler* );
  void (*takeoverFailtureHandler)( DPAUCS_fragment_t** );
} DPAUCS_fragmentHandler_t;

typedef struct DPAUCS_ip_packetInfo {
  const flash struct DPAUCS_fragmentHandler* handler; // must be the first member
  bool valid;
  uint16_t offset;
  void(*onremove)(void*);
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
DPAUCS_ip_packetInfo_t* DPAUCS_layer3_normalize_packet_info_ptr( DPAUCS_ip_packetInfo_t* );
DPAUCS_ip_packetInfo_t* DPAUCS_layer3_save_packet_info( DPAUCS_ip_packetInfo_t* );
void DPAUCS_layer3_removeFragment( DPAUCS_ip_fragment_t** );

#endif
