#ifndef DPAUCS_SERVER_H
#define DPAUCS_SERVER_H

#include <stdint.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>

#ifndef MAX_LOGIC_ADDRESSES
#define MAX_LOGIC_ADDRESSES 8
#endif

#ifndef MAX_SERVICES
#define MAX_SERVICES 16
#endif

struct DPAUCS_service;
struct DPAUCS_packet_info;
struct DPAUCS_ethernet_driver;

typedef struct DPAUCS_driver_info {
  const char* name;
  struct DPAUCS_ethernet_driver* driver;
} DPAUCS_driver_info_t;

void DPAUCS_run( void(*)(void*), void* );

NORETURN void DPAUCS_fatal( const char* message );
void DPAUCS_onfatalerror( const char* message );

void DPAUCS_add_logicAddress( const DPAUCS_interface_t*const interface, const DPAUCS_logicAddress_t*const logicAddress );
void DPAUCS_remove_logicAddress( const struct DPAUCS_logicAddress*const logicAddress );
void DPAUCS_each_logicAddress(enum DPAUCS_address_types, bool(*)(const struct DPAUCS_logicAddress*,void*), void*);
bool DPAUCS_has_logicAddress(const struct DPAUCS_logicAddress* addr);
const DPAUCS_logicAddress_t* DPAUCS_get_logicAddress( const DPAUCS_logicAddress_t* addr );

void DPAUCS_add_service( const struct DPAUCS_logicAddress* logicAddress, uint16_t port, struct DPAUCS_service* service );
struct DPAUCS_service* DPAUCS_get_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port, uint8_t tos );
void DPAUCS_remove_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port );
void DPAUCS_doNextTask( void );
const struct DPAUCS_interface* DPAUCS_getInterface( const struct DPAUCS_logicAddress* logicAddress );

void DPAUCS_preparePacket( struct DPAUCS_packet_info* info );
void DPAUCS_sendPacket( struct DPAUCS_packet_info* info, uint16_t size );

#endif
