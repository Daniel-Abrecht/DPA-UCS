#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <service.h>
#include <packet.h>
#include <protocol/address.h>

#ifndef MAX_LOGIC_ADDRESSES
#define MAX_LOGIC_ADDRESSES 8
#endif

#ifndef MAX_SERVICES
#define MAX_SERVICES 16
#endif

extern uint8_t mac[6];

void DPAUCS_run( void(*)(void*), void* );

NORETURN void DPAUCS_fatal( const char* message );
void DPAUCS_onfatalerror( const char* message );

void DPAUCS_add_logicAddress( const DPAUCS_logicAddress_t*const logicAddress );
void DPAUCS_remove_logicAddress( const DPAUCS_logicAddress_t*const logicAddress );
void DPAUCS_each_logicAddress(DPAUCS_address_types_t, bool(*)(const DPAUCS_logicAddress_t*,void*), void*);
bool DPAUCS_has_logicAddress(const DPAUCS_logicAddress_t* addr);

void DPAUCS_add_service( const DPAUCS_logicAddress_t* logicAddress, uint16_t port, DPAUCS_service_t* service );
DPAUCS_service_t* DPAUCS_get_service( const DPAUCS_logicAddress_t*const logicAddress, uint16_t port, uint8_t tos );
void DPAUCS_remove_service( const DPAUCS_logicAddress_t*const logicAddress, uint16_t port );
void DPAUCS_doNextTask( void );

// Internally used stuff //
void DPAUCS_preparePacket( DPAUCS_packet_info* info );
void DPAUCS_sendPacket( DPAUCS_packet_info* info, uint16_t size );
///////////////////////////

#endif
