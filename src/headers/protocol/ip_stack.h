#ifndef IP_STACK_H
#define IP_STACK_H

#include <stdint.h>

#define DEFAULT_TTL 64

typedef struct {
  uint32_t srcIp; // largest fields first to reduce padding
  uint32_t destIp;
  uint8_t srcMac[6];
  uint8_t destMac[6];  
  // packetNumber: enumerate packets, drop oldest if ip stack is full
  // to find out the oldest packet, use
  // packetNumber-packetNumberCounter instantof
  // packetNumber to take care about overflows
  unsigned short packetNumber;
  uint16_t id;
  uint16_t length;
  // destMac: always the same as global variable mac, only provided to simplify futur changes
  uint8_t offset;
} ip_packet_info;

extern unsigned short packetNumberCounter;

#endif
