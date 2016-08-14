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

#define DEFINE_ADDRESS_HANDLER_TYPE_( TYPE ) \
  typedef struct DPAUCS ## TYPE ## addressHandler { \
    uint16_t type; \
    bool (*isBroadcast)( const DPAUCS_logicAddress ## TYPE ## t* ); \
    bool (*isValid)( const DPAUCS_logicAddress ## TYPE ## t* ); \
    bool (*compare)( const DPAUCS_logicAddress ## TYPE ## t*, const DPAUCS_logicAddress ## TYPE ## t* ); \
    bool (*copy)( DPAUCS_logicAddress ## TYPE ## t*, const DPAUCS_logicAddress ## TYPE ## t* ); \
    bool (*withRawAsLogicAddress)( uint16_t, void*, size_t, void(*)(const DPAUCS_logicAddress ## TYPE ## t*,void*), void*); \
  } DPAUCS ## TYPE ## addressHandler_t

#define DEFINE_ADDRESS_HANDLER_TYPE( TYPE ) \
  DEFINE_ADDRESS_HANDLER_TYPE_( _ ## TYPE ## _ )

#define DPAUCS_EXPORT_ADDRESS_HANDLER( NAME, HANDLER ) \
  DPA_SECTION_LIST_ENTRY_HACK( const struct DPAUCS_ ## NAME ## _addressHandler*, DPAUCS_addressHandler, DPAUCS_ ## NAME ## _addressHandler ) HANDLER

#define DPAUCS_EACH_ADDRESS_HANDLER( ITERATOR ) \
  DPA_FOR_SECTION_LIST_HACK( const struct DPAUCS_addressHandler*, DPAUCS_addressHandler, ITERATOR )

DEFINE_ADDRESS_HANDLER_TYPE_(_);

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

const DPAUCS_addressHandler_t* DPAUCS_getAddressHandler( uint16_t type );
void DPAUCS_layer3_addProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_layer3_protocolHandler_t* handler );

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void );
bool DPAUCS_layer3_getPacketSizeLimit( enum DPAUCS_address_types, size_t* limit );
bool DPAUCS_layer3_transmit( DPA_stream_t* stream, const DPAUCS_mixedAddress_pair_t* fromTo, uint8_t type, size_t );
void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* x );

#endif
