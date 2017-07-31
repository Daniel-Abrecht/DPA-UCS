#include <stddef.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/protocol/layer3.h>

DPA_DEFINE_RINGBUFFER(unsigned char,DPA_uchar_ringbuffer_t,outputStreamBuffer,OUTSTREAM_BYTE_BUFFER_SIZE,false);
DPA_DEFINE_RINGBUFFER(DPA_bufferInfo_t,DPA_buffer_ringbuffer_t,outputStreamBufferBuffer,OUTSTREAM_REFERENCE_BUFFER_SIZE,false);

static DPA_stream_t outputStream = {
  &outputStreamBuffer,
  &outputStreamBufferBuffer
};

DPAUCS_l4_handler_t* l4_handlers[MAX_LAYER3_PROTO_HANDLERS];

void DPAUCS_layer3_addProtocolHandler(DPAUCS_l4_handler_t* handler){
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( !l4_handlers[i] ){
      l4_handlers[i] = handler;
      break;
    }
}

void DPAUCS_layer3_removeProtocolHandler(DPAUCS_l4_handler_t* handler){
  for(int i=0; i<MAX_LAYER3_PROTO_HANDLERS; i++)
    if( l4_handlers[i] == handler ){
      l4_handlers[i] = 0;
      break;
    }
}

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void ){
  return &outputStream;
}

void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* stream ){
  DPA_stream_reset( stream );
}

bool DPAUCS_layer3_transmit(
  size_t header_count,
  const size_t header_size[header_count],
  const void* header_data[header_count],
  DPA_stream_t* payload,
  const DPAUCS_mixedAddress_pair_t* fromTo,
  uint8_t type,
  size_t max_size
){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( DPAUCS_mixedPairGetType( fromTo ) );
  if( handler && handler->transmit )
    return (*handler->transmit)( header_count, header_size, header_data, payload, fromTo, type, max_size );
  return false;
}

void DPAUCS_layer3_packetHandler(  DPAUCS_packet_info_t* info ){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( info->type );
  if( handler && handler->packetHandler )
    (*handler->packetHandler)( info );
}

const DPAUCS_l3_handler_t* DPAUCS_getAddressHandler( uint16_t type ){
  DPAUCS_EACH_ADDRESS_HANDLER( it ){
    if( (*it)->type == type )
      return *it;
  }
  return 0;
}

bool DPAUCS_layer3_getPacketSizeLimit( uint16_t type, size_t* limit ){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( type );
  if( handler ){
    *limit = handler->packetSizeLimit;
    return true;
  }
  return false;
}
