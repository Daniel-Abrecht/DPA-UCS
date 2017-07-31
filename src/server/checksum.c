#include <DPA/utils/utils.h>
#include <DPA/utils/stream.h>
#include <DPA/UCS/checksum.h>

uint16_t checksum( void* p, size_t l ){
  uint32_t checksum = 0;
  unsigned unaligned = (uintptr_t)p & 1;
  uint16_t* ptr = (uint16_t*)((uintptr_t)p + unaligned);
  for(unsigned i=0;i<l/2-unaligned;i++){
    checksum += *(ptr++);
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  if( l & 1 ){
    checksum += DPA_htob16((uint16_t)*(char*)ptr<<8);
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  if( unaligned ){
    uint8_t* x = p;
    checksum += DPA_htob16(((uint16_t)x[0]<<8)|x[l-1]);
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  return ~checksum;
}

uint16_t checksumOfStream( DPA_stream_t* stream, size_t max_len ){
  uint32_t checksum = 0;
  if(!stream)
    return ~0;
  while( !DPA_stream_eof(stream) && max_len ){
    uint16_t num = 0;
    size_t s = DPA_MIN( sizeof(num), max_len );
    max_len -= s;
    DPA_stream_read( stream, &num, s );
    checksum += num;
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  return ~checksum;
}
