#include <server.h>
#include <protocol/ip.h>
#include <protocol/layer3.h>

//static buffer_buffer_t outputStreamBufferBuffer;
DEFINE_BUFFER(unsigned char,uchar_buffer_t,outputStreamBuffer,0);
DEFINE_BUFFER(bufferInfo_t,buffer_buffer_t,outputStreamBufferBuffer,3);

static stream_t outputStream = {
  &outputStreamBuffer,
  &outputStreamBufferBuffer,
  0,
  0
};

static struct {
  DPAUCS_address_pair_t* fromTo;
  unsigned fromTo_count;
  uint8_t type;
} currentTransmission;

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

stream_t* DPAUCS_layer3_transmissionBegin( DPAUCS_address_pair_t* fromTo, unsigned fromTo_count, uint8_t type ){
  currentTransmission.fromTo = fromTo;
  currentTransmission.fromTo_count = fromTo_count;
  currentTransmission.type = type;
  outputStream.outputStreamBufferWriteStartOffset = outputStreamBuffer.write_offset;
  outputStream.outputStreamBufferBufferWriteStartOffset = outputStreamBufferBuffer.write_offset;
  return &outputStream;
}

void DPAUCS_layer3_transmissionEnd(){

  uchar_buffer_t cb = outputStreamBuffer;
  buffer_buffer_t bb = outputStreamBufferBuffer;
  stream_t inputStream = { &cb, &bb, 0, 0 };

  for( unsigned i=0; i<currentTransmission.fromTo_count; i++ ){

    DPAUCS_address_pair_t* fromTo = currentTransmission.fromTo + i;

    // reset read offsets
    cb.read_offset = outputStreamBuffer.read_offset;
    bb.read_offset = outputStreamBufferBuffer.read_offset;

    switch( fromTo->source->type ){

      case AT_IPv4: DPAUCS_IPv4_transmit( 
        &inputStream,
        (const DPAUCS_IPv4_address_t*)fromTo->source,
        (const DPAUCS_IPv4_address_t*)fromTo->destination,
        currentTransmission.type
      ); break;

    }

  }

  outputStreamBufferBuffer.read_offset = outputStreamBufferBuffer.write_offset;
  outputStreamBuffer.read_offset = outputStreamBuffer.write_offset;

}
