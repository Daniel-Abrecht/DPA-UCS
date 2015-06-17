#include <stream.h>
#include <checksum.h>
#include <binaryUtils.h>

uint16_t checksum( void* p, size_t l ){
  uint32_t checksum = 0;
  unsigned unaligned = (uintptr_t)p & 1;
  uint16_t* ptr = (uint16_t*)((uintptr_t)p + unaligned);
  for(unsigned i=0;i<l/2-unaligned;i++){
    checksum += *(ptr++);
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  if( unaligned ){
    uint8_t* x = p;
    checksum += htob16(((uint16_t)x[0]<<8)|x[l-1]);
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  return ~checksum;
}

uint16_t checksumOfStream( DPAUCS_stream_t* stream ){
  uint32_t checksum = 0;
  while(!DPAUCS_stream_eof(stream)){
    uint16_t num = 0;
    DPAUCS_stream_read( stream, &num, sizeof(num) );
    checksum += num;
    checksum  = ( checksum & 0xFFFF ) + ( checksum >> 16 );
  }
  return ~checksum;
}
