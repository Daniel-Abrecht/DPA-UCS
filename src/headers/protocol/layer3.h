#ifndef LAYER3_H
#define LAYER3_H

#include <stream.h>
#include <protocol/address.h>

#define MAX_LAYER3_PROTO_HANDLERS 3

#ifndef OUTSTREAM_BYTE_BUFFER_SIZE
#define OUTSTREAM_BYTE_BUFFER_SIZE 256
#endif

#ifndef OUTSTREAM_REFERENCE_BUFFER_SIZE
#define OUTSTREAM_REFERENCE_BUFFER_SIZE 16
#endif

typedef struct DPAUCS_fragment DPAUCS_fragment_t;

typedef DPAUCS_stream_t*(*DPAUCS_createTransmissionStream)();
typedef void(*DPAUCS_transmit)( DPAUCS_stream_t* stream, DPAUCS_address_pair_t* fromTo, uint8_t type );
typedef void(*DPAUCS_destroyTransmissionStream)( DPAUCS_stream_t* );
typedef bool(*DPAUCS_layer3_ProtocolReciveHandler_func)(
  void* id,
  DPAUCS_address_t* from,
  DPAUCS_address_t* to,
  uint16_t,
  uint16_t,
  DPAUCS_fragment_t**,
  void*,
  bool
);
typedef void(*DPAUCS_layer3_ProtocolFailtureHandler_func)( void* id );

typedef struct {
  uint8_t protocol;
  DPAUCS_layer3_ProtocolReciveHandler_func onreceive;
  DPAUCS_layer3_ProtocolFailtureHandler_func onreceivefailture;
} DPAUCS_layer3_protocolHandler_t;

extern DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_LAYER3_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );

DPAUCS_stream_t* DPAUCS_layer3_createTransmissionStream();
void DPAUCS_layer3_transmit( DPAUCS_stream_t* stream, DPAUCS_address_pair_t* fromTo, uint8_t type );
void DPAUCS_layer3_destroyTransmissionStream( DPAUCS_stream_t* x );

#endif
