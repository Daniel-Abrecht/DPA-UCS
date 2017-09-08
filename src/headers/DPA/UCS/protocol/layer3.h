#ifndef DPAUCS_LAYER3_H
#define DPAUCS_LAYER3_H

#include <DPA/utils/stream.h>
#include <DPA/UCS/protocol/address.h>

#define MAX_LAYER4_PROTO_HANDLERS 3

#ifndef OUTSTREAM_BYTE_BUFFER_SIZE
#define OUTSTREAM_BYTE_BUFFER_SIZE 256
#endif

#ifndef OUTSTREAM_REFERENCE_BUFFER_SIZE
#define OUTSTREAM_REFERENCE_BUFFER_SIZE 16
#endif

typedef struct DPAUCS_fragment DPAUCS_fragment_t;
typedef struct DPAUCS_packet_info DPAUCS_packet_info_t;

typedef DPA_stream_t*(*DPAUCS_createTransmissionStream)( void );
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

typedef struct DPAUCS_l4_handler {
  uint8_t protocol;
  DPAUCS_layer3_ProtocolReciveHandler_func onreceive;
  DPAUCS_layer3_ProtocolFailtureHandler_func onreceivefailture;
} DPAUCS_l4_handler_t;

typedef struct DPAUCS_l3_handler {
  uint16_t type;
  size_t packetSizeLimit;
  size_t rawAddressSize;
  void (*packetHandler)( DPAUCS_packet_info_t* );
  bool (*isBroadcast)( const DPAUCS_logicAddress_t* );
  bool (*isValid)( const DPAUCS_logicAddress_t* );
  bool (*compare)( const DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t* );
  bool (*copy)( DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t* );
  bool (*withRawAsLogicAddress)( void*, size_t, void(*)(const DPAUCS_logicAddress_t*,void*), void*);
  bool (*transmit)( size_t, const size_t[], const void*[], DPA_stream_t*, const DPAUCS_mixedAddress_pair_t*, uint8_t, size_t );
  uint16_t (*calculatePseudoHeaderChecksum)( const DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t*, uint8_t, uint16_t );
} DPAUCS_l3_handler_t;

DPA_LOOSE_LIST_DECLARE( const flash struct DPAUCS_l3_handler*, DPAUCS_l3_handler_list )

extern DPAUCS_l4_handler_t* l4_handlers[MAX_LAYER4_PROTO_HANDLERS];

const flash DPAUCS_l3_handler_t* DPAUCS_getAddressHandler( uint16_t type );
void DPAUCS_layer3_addProtocolHandler( DPAUCS_l4_handler_t* handler );
void DPAUCS_layer3_removeProtocolHandler( DPAUCS_l4_handler_t* handler );

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void );
bool DPAUCS_layer3_getPacketSizeLimit( uint16_t, size_t* limit );
bool DPAUCS_layer3_transmit(
  size_t header_count,
  const size_t header_size[header_count],
  const void* header_data[header_count],
  DPA_stream_t* payload,
  const DPAUCS_mixedAddress_pair_t* fromTo,
  uint8_t type,
  size_t
);
void DPAUCS_layer3_packetHandler(  DPAUCS_packet_info_t* info );
void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* x );

#endif
