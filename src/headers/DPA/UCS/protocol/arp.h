#ifndef DPAUCS_ARP_H
#define DPAUCS_ARP_H

#include <DPA/UCS/helper_macros.h>

struct DPAUCS_packet_info;
struct DPAUCS_address;
struct DPAUCS_logicAddress;

#define ARP_REQUEST 1
#define ARP_RESPONSE 2

#ifndef ARP_ENTRY_COUNT
#define ARP_ENTRY_COUNT 64
#endif


typedef PACKED1 struct PACKED2 {
  uint16_t htype; // Hardware type
  uint16_t ptype; // Protocol type
  uint8_t hlen; // Hardware address length
  uint8_t plen; // Protocol address length
  uint16_t oper; // Operation
  // source mac
  // source protocol address
  // target mac
  // target protocol address
} DPAUCS_arp_t;

const DPAUCS_address_t* DPAUCS_arpCache_register( const struct DPAUCS_address* );
bool DPAUCS_arpCache_deregister( const struct DPAUCS_logicAddress* );
void DPAUCS_arp_handler( struct DPAUCS_packet_info* info );
DPAUCS_address_t* DPAUCS_arpCache_getAddress( const struct DPAUCS_logicAddress* );

#endif
