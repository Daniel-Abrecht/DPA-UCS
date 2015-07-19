#include <server.h>
#include <protocol/ip.h>
#include <protocol/layer3.h>

DEFINE_BUFFER(unsigned char,uchar_buffer_t,outputStreamBuffer,0);
DEFINE_BUFFER(bufferInfo_t,buffer_buffer_t,outputStreamBufferBuffer,3);

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

DPAUCS_stream_t* DPAUCS_layer3_createTransmissionStream(){
  return &outputStream;
}

void DPAUCS_layer3_transmit( DPAUCS_stream_t* stream, DPAUCS_address_pair_t* fromTo, uint8_t type ){

  DPAUCS_stream_offsetStorage_t sros;
  DPAUCS_stream_saveReadOffset( &sros, stream );

  switch( fromTo->source->type ){

#ifdef USE_IPv4
    case AT_IPv4: DPAUCS_IPv4_transmit( 
      stream,
      (const DPAUCS_IPv4_address_t*)fromTo->source,
      (const DPAUCS_IPv4_address_t*)fromTo->destination,
      type
    ); break;
#endif

  }

  DPAUCS_stream_restoreReadOffset( stream, &sros );

}

void DPAUCS_layer3_destroyTransmissionStream( DPAUCS_stream_t* stream ){
  DPAUCS_stream_reset( stream );
}
