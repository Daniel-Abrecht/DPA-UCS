#ifndef DPAUCS_ETH_DRIVER
#define DPAUCS_ETH_DRIVER

#include <stdint.h>
#include <DPA/UCS/protocol/address.h>

#define DPAUCS_ETHERNET_DRIVER_SYMBOL( NAME ) \
  DPAUCS_ethernet_driver_ ## NAME

#define DPAUCS_ETHERNET_DRIVER_DECLARATION( NAME ) \
  DPAUCS_ethernet_driver_t DPAUCS_ETHERNET_DRIVER_SYMBOL( NAME )

#define DPAUCS_ETHERNET_DRIVER( NAME ) \
  extern DPAUCS_ETHERNET_DRIVER_DECLARATION( NAME ); \
  DPAUCS_ETHERNET_DRIVER_DECLARATION( NAME ) =

typedef struct DPAUCS_ethernet_driver {

  void(*init)( void );
  void(*send)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t len );
  uint16_t(*receive)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t maxlen );
  void(*shutdown)( void );

  DPAUCS_interface_t* interfaces;
  unsigned interface_count;

} DPAUCS_ethernet_driver_t;

#endif
