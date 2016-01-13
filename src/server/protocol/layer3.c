#include <stddef.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/ip.h>
#include <DPA/UCS/protocol/layer3.h>

DPAUCS_DEFINE_BUFFER(unsigned char,DPAUCS_uchar_buffer_t,outputStreamBuffer,OUTSTREAM_BYTE_BUFFER_SIZE,false);
DPAUCS_DEFINE_BUFFER(DPAUCS_bufferInfo_t,DPAUCS_buffer_buffer_t,outputStreamBufferBuffer,OUTSTREAM_REFERENCE_BUFFER_SIZE,false);

static DPAUCS_stream_t outputStream = {
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

DPAUCS_stream_t* DPAUCS_layer3_createTransmissionStream( void ){
  return &outputStream;
}

void DPAUCS_layer3_transmit( DPAUCS_stream_t* stream, DPAUCS_address_pair_t* fromTo, uint8_t type, size_t max_size ){

  switch( fromTo->source->type ){

#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: DPAUCS_IPv4_transmit(
      stream,
      (const DPAUCS_IPv4_address_t*)fromTo->source,
      (const DPAUCS_IPv4_address_t*)fromTo->destination,
      type,
      max_size
    ); break;
#endif

  }

}

bool DPAUCS_layer3_getPacketSizeLimit( enum DPAUCS_address_types type, size_t* limit ){
  switch( type ){

#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: *limit = 0xFFFFu; return true;
#endif

  }
  return false;
}

void DPAUCS_layer3_destroyTransmissionStream( DPAUCS_stream_t* stream ){
  DPAUCS_stream_reset( stream );
}
