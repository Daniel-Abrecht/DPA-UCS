#ifndef DPAUCS_ETH_DRIVER_H
#define DPAUCS_ETH_DRIVER_H

#include <stdint.h>
#include <DPA/UCS/protocol/address.h>

typedef struct DPAUCS_ethernet_driver {

  const char* name;

  void(*init)( void );
  void(*send)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t len );
  uint16_t(*receive)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t maxlen );
  void(*shutdown)( void );

  DPAUCS_interface_t* interfaces;
  unsigned interface_count;

} DPAUCS_ethernet_driver_t;

DPA_LOOSE_LIST_DECLARE( struct DPAUCS_ethernet_driver*, DPAUCS_ethernet_driver_list )

#endif
