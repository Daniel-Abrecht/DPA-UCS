#ifndef ANY_ADDRESS_H
#define ANY_ADDRESS_H

#include <protocol/address.h>
#include <protocol/IPv4.h>

typedef union {
  DPAUCS_logicAddress_t address;
  DPAUCS_logicAddress_IPv4_t IPv4;
} DPAUCS_any_logicAddress_t;

#endif