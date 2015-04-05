#ifndef ARP_H
#define ARP_H

#include <packet.h>
#include <packed.h>

#define ARP_REQUEST 1
#define ARP_RESPONSE 2

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


void DPAUCS_arp_handler(DPAUCS_packet_info* info);

#endif
