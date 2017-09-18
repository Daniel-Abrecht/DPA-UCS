#ifndef DPAUCS_ETH_DRIVER_H
#define DPAUCS_ETH_DRIVER_H

#include <stdint.h>
#include <DPA/UCS/protocol/address.h>

struct DPAUCS_interface_list;
struct DPAUCS_interface_list {
  DPAUCS_interface_t* entry;
  struct DPAUCS_interface_list* next;
};

typedef struct DPAUCS_ethernet_driver {

  const char*const name;

  void(*init)( void );
  void(*send)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t len );
  uint16_t(*receive)( const struct DPAUCS_interface* interface, uint8_t* packet, uint16_t maxlen );
  void(*shutdown)( void );

  struct DPAUCS_interface_list* interface_list;

} DPAUCS_ethernet_driver_t;

DPA_LOOSE_LIST_DECLARE( struct DPAUCS_ethernet_driver*, DPAUCS_ethernet_driver_list )

enum DPAUCS_interface_event {
  DPAUCS_IFACE_EVENT_LINK_UP,
  DPAUCS_IFACE_EVENT_LINK_DOWN
};

typedef void(*event_handler)( DPAUCS_interface_t*, enum DPAUCS_interface_event, void* );
void DPAUCS_interface_event( DPAUCS_interface_t*, enum DPAUCS_interface_event, void* param );

DPA_LOOSE_LIST_DECLARE( event_handler, DPAUCS_event_list )

#endif
