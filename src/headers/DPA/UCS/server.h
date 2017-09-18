#ifndef DPAUCS_SERVER_H
#define DPAUCS_SERVER_H

#include <stdint.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/protocol/address.h>

#ifndef MAX_LOGIC_ADDRESSES
#define MAX_LOGIC_ADDRESSES 8
#endif

#ifndef MAX_SERVICES
#define MAX_SERVICES 16
#endif

#define DPAUCS_INIT \
  static void DPA_CONCAT( DPA_CONCAT( DPAUCS_init_, __LINE__ ), _func )( void ); \
  DPA_LOOSE_LIST_ADD( DPAUCS_init_function_list, &DPA_CONCAT( DPA_CONCAT( DPAUCS_init_, __LINE__ ), _func ) ) \
  static void DPA_CONCAT( DPA_CONCAT( DPAUCS_init_, __LINE__ ), _func )( void )

#define DPAUCS_SHUTDOWN \
  static void DPA_CONCAT( DPA_CONCAT( DPAUCS_shutdown_, __LINE__ ), _func )( void ); \
  DPA_LOOSE_LIST_ADD( DPAUCS_shutdown_function_list, &DPA_CONCAT( DPA_CONCAT( DPAUCS_shutdown_, __LINE__ ), _func ) ) \
  static void DPA_CONCAT( DPA_CONCAT( DPAUCS_shutdown_, __LINE__ ), _func )( void )

typedef void(*DPAUCS_init_func_t)(void);
typedef void(*DPAUCS_shutdown_func_t)(void);

DPA_LOOSE_LIST_DECLARE( const DPAUCS_init_func_t, DPAUCS_init_function_list )
DPA_LOOSE_LIST_DECLARE( const DPAUCS_shutdown_func_t, DPAUCS_shutdown_function_list )

struct DPAUCS_service;
struct DPAUCS_packet_info;
struct DPAUCS_ethernet_driver;

void DPAUCS_run( void(*)(void*), void* );

noreturn void DPAUCS_fatal( const flash char* message );
void DPAUCS_onfatalerror( const flash char* message );

void DPAUCS_add_logicAddress( const DPAUCS_interface_t*const interface, const DPAUCS_logicAddress_t*const logicAddress );
void DPAUCS_remove_logicAddress( const struct DPAUCS_logicAddress*const logicAddress );
void DPAUCS_each_logicAddress(uint16_t, bool(*)(const struct DPAUCS_logicAddress*,void*), void*);
bool DPAUCS_has_logicAddress(const struct DPAUCS_logicAddress* addr);
const DPAUCS_logicAddress_t* DPAUCS_get_logicAddress( const DPAUCS_logicAddress_t* addr );

bool DPAUCS_add_service( const struct DPAUCS_logicAddress* logicAddress, uint16_t port, const flash struct DPAUCS_service* service );
const flash struct DPAUCS_service* DPAUCS_get_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port, uint8_t tos );
bool DPAUCS_remove_service( const struct DPAUCS_logicAddress*const logicAddress, uint16_t port );
void DPAUCS_doNextTask( void );
const struct DPAUCS_interface* DPAUCS_getInterface( const struct DPAUCS_logicAddress* logicAddress );

void DPAUCS_preparePacket( struct DPAUCS_packet_info* info );
void DPAUCS_sendPacket( struct DPAUCS_packet_info* info, uint16_t size );

#endif
