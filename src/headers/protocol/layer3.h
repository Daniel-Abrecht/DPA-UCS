#ifndef LAYER3_H
#define LAYER3_H

#include <stream.h>
#include <protocol/address.h>

#define MAX_LAYER3_PROTO_HANDLERS 3

typedef stream_t*(*DPAUCS_beginTransmission)( DPAUCS_address_pair_t* fromTo, unsigned fromTo_count, uint8_t type );
typedef void(*DPAUCS_endTransmission)();
typedef bool(*DPAUCS_layer3_ProtocolReciveHandler_func)( void* id, DPAUCS_address_t* from, DPAUCS_address_t* to, DPAUCS_beginTransmission beginTransmission, DPAUCS_endTransmission, uint16_t, uint16_t, void*, bool );
typedef void(*DPAUCS_layer3_ProtocolFailtureHandler_func)( void* id );

typedef struct {
  uint8_t protocol;
  DPAUCS_layer3_ProtocolReciveHandler_func onrecive;
  DPAUCS_layer3_ProtocolFailtureHandler_func onrecivefailture;
} DPAUCS_layer3_protocolHandler_t;

extern DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_LAYER3_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );

stream_t* DPAUCS_layer3_transmissionBegin( DPAUCS_address_pair_t* fromTo, unsigned fromTo_count, uint8_t type );
void DPAUCS_layer3_transmissionEnd();

#endif
