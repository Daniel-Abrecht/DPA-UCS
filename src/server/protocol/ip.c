#include <server.h>
#include <protocol/ip.h>

DPAUCS_layer3_protocolHandler_t* layer3_protocolHandlers[MAX_IP_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler(DPAUCS_layer3_protocolHandler_t* handler){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( !layer3_protocolHandlers[i] ){
      layer3_protocolHandlers[i] = handler;
      break;
    }
}

void DPAUCS_layer3_removeProtocolHandler(DPAUCS_layer3_protocolHandler_t* handler){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( layer3_protocolHandlers[i] == handler ){
      layer3_protocolHandlers[i] = 0;
      break;
    }
}

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
  DPAUCS_address_t* from;
  DPAUCS_address_t** to;
  uint8_t type;
} currentTransmission;

stream_t* DPAUCS_layer3_transmissionBegin( DPAUCS_address_t* from, DPAUCS_address_t** to, uint8_t type ){
  currentTransmission.from = from;
  currentTransmission.to = to;
  currentTransmission.type = type;
  outputStream.outputStreamBufferWriteStartOffset = outputStreamBuffer.write_offset;
  outputStream.outputStreamBufferBufferWriteStartOffset = outputStreamBufferBuffer.write_offset;
  return &outputStream;
}

void DPAUCS_layer3_transmissionEnd(){

  uchar_buffer_t cb = outputStreamBuffer;
  buffer_buffer_t bb = outputStreamBufferBuffer;
  stream_t inputStream = { &cb, &bb, 0, 0 };

  for( DPAUCS_address_t** to=currentTransmission.to; *to; to++ ){

    // reset read offsets
    cb.read_offset = outputStreamBuffer.read_offset;
    bb.read_offset = outputStreamBufferBuffer.read_offset;

    switch((*to)->type){
      case AT_IPv4: DPAUCS_IPv4_transmit( &inputStream, (const DPAUCS_ipv4_address_t*)currentTransmission.from, (const DPAUCS_ipv4_address_t*)*to, currentTransmission.type ); break;
    }

  }

  outputStreamBufferBuffer.read_offset = outputStreamBufferBuffer.write_offset;
  outputStreamBuffer.read_offset = outputStreamBuffer.write_offset;

}

void DPAUCS_ip_handler( DPAUCS_packet_info* info ){
  DPAUCS_ip_t* ip = info->payload;
  switch( ip->version >> 4 ){
    case 4: DPAUCS_IPv4_handler(info,&ip->ipv4); break;
  }
}

