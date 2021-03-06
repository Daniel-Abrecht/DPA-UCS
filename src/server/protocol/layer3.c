#include <stddef.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/packet.h>
#include <DPA/utils/utils.h>
#include <DPA/UCS/protocol/layer3.h>

DPA_DEFINE_RINGBUFFER(unsigned char,DPA_uchar_ringbuffer_t,outputStreamBuffer,OUTSTREAM_BYTE_BUFFER_SIZE,false);
DPA_DEFINE_RINGBUFFER(DPA_bufferInfo_t,DPA_buffer_ringbuffer_t,outputStreamBufferBuffer,OUTSTREAM_REFERENCE_BUFFER_SIZE,false);

static DPA_stream_t outputStream = {
  &outputStreamBuffer,
  &outputStreamBufferBuffer
};

DPA_stream_t* DPAUCS_layer3_createTransmissionStream( void ){
  return &outputStream;
}

void DPAUCS_layer3_destroyTransmissionStream( DPA_stream_t* stream ){
  DPA_stream_reset( stream );
}

bool DPAUCS_layer3_transmit(
  const linked_data_list_t* headers,
  DPA_stream_t* payload,
  const DPAUCS_mixedAddress_pair_t* fromTo,
  uint8_t type,
  size_t max_size
){
  const flash DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( DPAUCS_mixedPairGetType( fromTo ) );
  if( handler && handler->transmit )
    return (*handler->transmit)( headers, payload, fromTo, type, max_size );
  return false;
}

void DPAUCS_layer3_packetHandler(  DPAUCS_packet_info_t* info ){
  const flash DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( info->type );
  if( handler && handler->packetHandler )
    (*handler->packetHandler)( info );
}

const flash DPAUCS_l3_handler_t* DPAUCS_getAddressHandler( uint16_t type ){
  for( struct DPAUCS_l3_handler_list* it = DPAUCS_l3_handler_list; it; it=it->next ){
    if( it->entry->type == type )
      return it->entry;
  }
  return 0;
}

bool DPAUCS_layer3_getPacketSizeLimit( uint16_t type, size_t* limit ){
  const flash DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( type );
  if( handler ){
    *limit = handler->packetSizeLimit;
    return true;
  }
  return false;
}
