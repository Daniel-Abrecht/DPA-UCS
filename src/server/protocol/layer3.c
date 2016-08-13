#include <stddef.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>

DPAUCS_DEFINE_RINGBUFFER(unsigned char,DPA_uchar_ringbuffer_t,outputStreamBuffer,OUTSTREAM_BYTE_BUFFER_SIZE,false);
DPAUCS_DEFINE_RINGBUFFER(DPA_bufferInfo_t,DPA_buffer_ringbuffer_t,outputStreamBufferBuffer,OUTSTREAM_REFERENCE_BUFFER_SIZE,false);

static DPA_stream_t outputStream = {
  &outputStreamBuffer,
  &outputStreamBufferBuffer
};

DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_LAYER3_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler(DPAUCS_layer3_protocolHandler_t* handler){
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( !layer3_protocolHandlers[i] ){
      layer3_protocolHandlers[i] = handler;
      break;
    }
}

void DPAUCS_layer3_removeProtocolHandler(DPAUCS_layer3_protocolHandler_t* handler){
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( layer3_protocolHandlers[i] == handler ){
      layer3_protocolHandlers[i] = 0;
      break;
    }
}

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void ){
  return &outputStream;
}

bool DPAUCS_layer3_transmit( DPA_stream_t* stream, const DPAUCS_mixedAddress_pair_t* fromTo, uint8_t type, size_t max_size ){

  switch( DPAUCS_mixedPairGetType( fromTo ) ){

#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: return DPAUCS_IPv4_transmit(
      stream,
      fromTo,
      type,
      max_size
    );
#endif
    case DPAUCS_AT_UNKNOWN: break;

  }

  return false;
}

bool DPAUCS_layer3_getPacketSizeLimit( enum DPAUCS_address_types type, size_t* limit ){
  switch( type ){

#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: *limit = 0xFFFFu; return true;
#endif
    case DPAUCS_AT_UNKNOWN: break;

  }
  return false;
}

void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* stream ){
  DPA_stream_reset( stream );
}
