#ifndef DPAUCS_ARP_H
#define DPAUCS_ARP_H

struct DPAUCS_packet_info;
struct DPAUCS_address;
struct DPAUCS_logicAddress;

#ifndef ARP_ENTRY_BUFFER_SIZE
#define ARP_ENTRY_BUFFER_SIZE 256
#endif

#define DPAUCS_ETH_T_ARP 0x0806

typedef struct packed {
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
DPAUCS_address_t* DPAUCS_arpCache_getAddress( const struct DPAUCS_logicAddress* );

#endif
