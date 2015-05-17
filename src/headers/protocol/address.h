#ifndef ADDRESS_TYPES_H
#define ADDRESS_TYPES_H

typedef enum {
  AT_IPv4
} DPAUCS_address_types_t;

typedef struct {
  DPAUCS_address_types_t type;
  uint8_t mac[6];
} DPAUCS_address_t;

#endif