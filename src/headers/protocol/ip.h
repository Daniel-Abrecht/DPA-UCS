#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <packet.h>
#include <packed.h>
#include <stream.h>
#include <protocol/ipv4.h>
#include <protocol/ipv6.h>
#include <protocol/address.h>

#define MAX_IP_PROTO_HANDLERS 3

typedef PACKED1 union PACKED2 {
  uint8_t version;
  DPAUCS_ipv4_t ipv4;
  DPAUCS_ipv6_t ipv6;
} DPAUCS_ip_t;

void DPAUCS_ip_handler( DPAUCS_packet_info* info );

typedef stream_t*(*DPAUCS_beginTransmission)( DPAUCS_address_t* from, DPAUCS_address_t** to, uint8_t type );
typedef void(*DPAUCS_endTransmission)();
typedef bool(*DPAUCS_ipProtocolReciveHandler_func)( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_beginTransmission beginTransmission, DPAUCS_endTransmission, uint16_t, uint16_t, void*, bool );
typedef void(*DPAUCS_ipProtocolFailtureHandler_func)( void* id );

typedef struct {
  uint8_t protocol;
  DPAUCS_ipProtocolReciveHandler_func onrecive;
  DPAUCS_ipProtocolFailtureHandler_func onrecivefailture;
} DPAUCS_layer3_protocolHandler_t;

extern DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_IP_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );

stream_t* DPAUCS_layer3_transmissionBegin( DPAUCS_address_t* from, DPAUCS_address_t** to, uint8_t type );
void DPAUCS_layer3_transmissionEnd();

#endif
