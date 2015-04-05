#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <service.h>
#include <packet.h>

#define IP_ANY 0
#define IP_BROADCAST (~(uint32_t)0)

#ifndef MAX_IPS
#define MAX_IPS 8
#endif

#ifndef MAX_SERVICES
#define MAX_SERVICES 16
#endif


extern const uint8_t mac[6];
extern uint32_t ips[MAX_IPS];


void DPAUCS_init( void );
void DPAUCS_shutdown( void );
void DPAUCS_add_ip( uint32_t ip );
void DPAUCS_remove_ip( uint32_t ip );
void DPAUCS_add_service( uint32_t ip, uint16_t port, DPAUCS_service_t* service );
void DPAUCS_remove_service( uint32_t ip, uint16_t port );
void DPAUCS_do_next_task( void );


// Internally used stuff //
void DPAUCS_preparePacket( DPAUCS_packet_info* info );
void DPAUCS_sendEth( DPAUCS_packet_info* info, uint16_t size );
///////////////////////////

#endif
