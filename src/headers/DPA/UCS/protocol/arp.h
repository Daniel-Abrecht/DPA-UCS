#ifndef ARP_H
#define ARP_H

#include <DPA/UCS/packet.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>

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

DPAUCS_address_t* DPAUCS_arpCache_register( DPAUCS_address_t* );
bool DPAUCS_arpCache_deregister( DPAUCS_address_t* );
void DPAUCS_arp_handler( struct DPAUCS_packet_info* info );

#endif
