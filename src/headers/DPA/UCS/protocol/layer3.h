#ifndef DPAUCS_LAYER3_H
#define DPAUCS_LAYER3_H

#include <DPA/utils/stream.h>
#include <DPA/UCS/protocol/address.h>

#define MAX_LAYER3_PROTO_HANDLERS 3

#ifndef OUTSTREAM_BYTE_BUFFER_SIZE
#define OUTSTREAM_BYTE_BUFFER_SIZE 256
#endif

#ifndef OUTSTREAM_REFERENCE_BUFFER_SIZE
#define OUTSTREAM_REFERENCE_BUFFER_SIZE 16
#endif

typedef struct DPAUCS_fragment DPAUCS_fragment_t;

typedef DPA_stream_t*(*DPAUCS_createTransmissionStream)( void );
typedef void(*DPAUCS_transmit)( DPA_stream_t* stream, DPAUCS_address_pair_t* fromTo, uint8_t type );
typedef void(*DPAUCS_destroyTransmissionStream)( DPA_stream_t* );
typedef bool(*DPAUCS_layer3_ProtocolReciveHandler_func)(
  void* id,
  struct DPAUCS_address* from,
  struct DPAUCS_address* to,
  uint16_t,
  uint16_t,
  struct DPAUCS_fragment**,
  void*,
  bool
);
typedef void(*DPAUCS_layer3_ProtocolFailtureHandler_func)( void* id );

typedef struct DPAUCS_layer3_protocolHandler {
  uint8_t protocol;
  DPAUCS_layer3_ProtocolReciveHandler_func onreceive;
  DPAUCS_layer3_ProtocolFailtureHandler_func onreceivefailture;
} DPAUCS_layer3_protocolHandler_t;

extern DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_LAYER3_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void );
bool DPAUCS_layer3_getPacketSizeLimit( enum DPAUCS_address_types, size_t* limit );
bool DPAUCS_layer3_transmit( DPA_stream_t* stream, const DPAUCS_mixedAddress_pair_t* fromTo, uint8_t type, size_t );
void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* x );

#endif
