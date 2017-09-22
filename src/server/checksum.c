#include <DPA/utils/utils.h>
#include <DPA/utils/stream.h>
#include <DPA/UCS/checksum.h>

uint16_t checksum( const void* p, size_t l ){
  if( !l )
    return ~0;
  uint32_t checksum = 0;
  const uint8_t* ptr = p;
  for( size_t i=0; i<l; i++ ){
    // The cast is necessary on systems where int = 16bit because of integer promotion and signedness
    checksum += (i%2) ? (uint16_t)ptr[i] : ((uint16_t)ptr[i])<<8;
  }
  checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  checksum = (uint16_t)~DPA_htob16(checksum);
  return checksum;
}

uint16_t checksumOfStream( DPA_stream_t* stream, size_t max_len ){
  uint32_t checksum = 0;
  if(!stream)
    return ~0;
  size_t size = 0;
  while( !DPA_stream_eof(stream) && max_len ){
    uint16_t num = 0;
    size_t s = DPA_MIN( sizeof(num), max_len );
    max_len -= s;
    size += s;
    DPA_stream_read( stream, &num, s );
    checksum += num;
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  DPA_stream_restoreReadOffset( stream, size );
  return ~checksum;
}
