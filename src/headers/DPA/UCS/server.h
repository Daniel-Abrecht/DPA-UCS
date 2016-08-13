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

#define DPAUCS_INIT( NAME ) \
  static void DPAUCS_real_init_ ## NAME ## _func(void); \
  DPA_SECTION_LIST_ENTRY_HACK( const DPAUCS_init_func_t, DPAUCS_init_func, DPAUCS_ ## NAME ## _init ) &DPAUCS_real_init_ ## NAME ## _func; \
  void DPAUCS_real_init_ ## NAME ## _func(void)

#define DPAUCS_SHUTDOWN( NAME ) \
  static void DPAUCS_real_shutdown_ ## NAME ## _func(void); \
  DPA_SECTION_LIST_ENTRY_HACK( const DPAUCS_shutdown_func_t, DPAUCS_shutdown_func, DPAUCS_ ## NAME ## _shutdown ) &DPAUCS_real_shutdown_ ## NAME ## _func; \
  void DPAUCS_real_shutdown_ ## NAME ## _func(void)

#define DPAUCS_EACH_INIT_FUNCTION( ITERATOR ) \
  DPA_FOR_SECTION_LIST_HACK( const DPAUCS_init_func_t, DPAUCS_init_func, ITERATOR )

#define DPAUCS_EACH_SHUTDOWN_FUNCTION( ITERATOR ) \
  DPA_FOR_SECTION_LIST_HACK( const DPAUCS_shutdown_func_t, DPAUCS_shutdown_func, ITERATOR )


typedef void(*DPAUCS_init_func_t)(void);
typedef void(*DPAUCS_shutdown_func_t)(void);

struct DPAUCS_service;
struct DPAUCS_packet_info;
struct DPAUCS_ethernet_driver;

void DPAUCS_run( void(*)(void*), void* );

noreturn void DPAUCS_fatal( const char* message );
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
