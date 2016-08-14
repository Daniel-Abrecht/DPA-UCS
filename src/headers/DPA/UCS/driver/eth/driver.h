#ifndef DPAUCS_ETH_DRIVER_H
#define DPAUCS_ETH_DRIVER_H

#include <stdint.h>
#include <DPA/UCS/protocol/address.h>

#define DPAUCS_EXPORT_ETHERNET_DRIVER( NAME, DRIVER ) \
  DPA_SECTION_LIST_ENTRY_HACK( const struct DPAUCS_ethernet_driver_entry, DPAUCS_ethernet_driver_entry, DPAUCS_ethernet_driver_ ## NAME ){ #NAME, DRIVER }

#define DPAUCS_EACH_ETHERNET_DRIVER( ITERATOR ) \
  DPA_FOR_SECTION_LIST_HACK( const struct DPAUCS_ethernet_driver_entry, DPAUCS_ethernet_driver_entry, ITERATOR )

#define DPAUCS_ETHERNET_DRIVER_GET_LIST( START, END ) \
  DPA_FOR_SECTION_GET_LIST( const struct DPAUCS_ethernet_driver_entry, DPAUCS_ethernet_driver_entry, START, END )

typedef struct DPAUCS_ethernet_driver {

  void(*init)( void );
  void(*send)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t len );
  uint16_t(*receive)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t maxlen );
  void(*shutdown)( void );

  DPAUCS_interface_t* interfaces;
  unsigned interface_count;

} DPAUCS_ethernet_driver_t;

typedef struct DPAUCS_ethernet_driver_entry {
  const char* name;
  const struct DPAUCS_ethernet_driver* driver;
} DPAUCS_ethernet_driver_entry_t;

#endif
