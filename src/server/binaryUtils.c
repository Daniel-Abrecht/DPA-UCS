#include <DPA/UCS/binaryUtils.h>

uint16_t btoh16(uint16_t y){
  uint8_t* x = (uint8_t*)&y;
  return ( (uint16_t)x[0] << 8u )
       | ( (uint16_t)x[1] );
}

uint32_t btoh32(uint32_t y){
  uint8_t* x = (uint8_t*)&y;
  return ( (uint32_t)x[0] << 24u ) |
         ( (uint32_t)x[1] << 16u ) |
         ( (uint32_t)x[2] <<  8u ) |
         ( (uint32_t)x[3] );
}

