#ifndef IP_STACK_H
#define IP_STACK_H

#include <stdint.h>
#include <protocol/tcp_ip_stack_memory.h>

#define DEFAULT_TTL 64

typedef struct { 
  DPAUCS_fragment_t fragment_info; // must be the first member

  uint32_t srcIp; // largest fields first to reduce padding
  uint32_t destIp;
  uint8_t srcMac[6];
  // destMac: always the same as global variable mac, only provided to simplify futur changes
  uint8_t destMac[6];
  uint16_t id;
  uint16_t offset;
  uint8_t tos; // Type of Service

} DPAUCS_ip_fragment_t;

bool DPAUCS_isNextIpFragment(DPAUCS_ip_fragment_t*);
DPAUCS_ip_fragment_t* DPAUCS_searchFollowingIpFragment(DPAUCS_ip_fragment_t*);

#endif
