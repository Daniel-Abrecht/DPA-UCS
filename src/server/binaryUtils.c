#include <binaryUtils.h>

uint16_t btoh16(uint16_t x){
  uint8_t y[] = {
    ( x >> 8 ) & 0xFF,
    x & 0xFF
  };
  return *(uint16_t*)y;
}

uint32_t btoh32(uint32_t x){
  uint8_t y[] = {
    ( x >> 24 ) & 0xFF,
    ( x >> 16 ) & 0xFF,
    ( x >>  8 ) & 0xFF,
      x & 0xFF
  };
  return *(uint32_t*)y;
}

