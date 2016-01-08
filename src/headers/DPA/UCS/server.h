#ifndef SERVER_H
#define SERVER_H

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

extern uint8_t mac[6];

void DPAUCS_run( void(*)(void*), void* );

NORETURN void DPAUCS_fatal( const char* message );
void DPAUCS_onfatalerror( const char* message );

void DPAUCS_add_logicAddress( const struct DPAUCS_logicAddress*const logicAddress );
void DPAUCS_remove_logicAddress( const struct DPAUCS_logicAddress*const logicAddress );
void DPAUCS_each_logicAddress(enum DPAUCS_address_types, bool(*)(const struct DPAUCS_logicAddress*,void*), void*);
bool DPAUCS_has_logicAddress(const struct DPAUCS_logicAddress* addr);

void DPAUCS_add_service( const struct DPAUCS_logicAddress* logicAddress, uint16_t port, struct DPAUCS_service* service );
struct DPAUCS_service* DPAUCS_get_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port, uint8_t tos );
void DPAUCS_remove_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port );
void DPAUCS_doNextTask( void );

// Internally used stuff //
void DPAUCS_preparePacket( struct DPAUCS_packet_info* info );
void DPAUCS_sendPacket( struct DPAUCS_packet_info* info, uint16_t size );
///////////////////////////

#endif
