#include <stdint.h>
#include <DPA/utils/encoding/base64.h>
#include <DPA/utils/helper_macros.h>

static const flash char base64chars[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="};

size_t DPA_encoding_base64_getLength( size_t n ){
  return ( n + 2 ) / 3 * 4;
}

void DPA_encoding_base64_encode( void* out, size_t m, const void* in, size_t n ){
  const char* s = in;
  char* o = out;
  for( ; n && m; s+=3, o+=4 ){
    uint8_t r[4] = {64,64,64,64};
    switch( n ){
      default: n--;
        r[3] =   s[2] & 0x3F;
        r[2] = ( s[2] >> 6 ) & 0x03;
      case 2: n--;
        r[2] = ( r[2] & 0x03 ) | ( ( s[1] & 0x0F ) << 2 );
        r[1] = ( s[1] >> 4 ) & 0x0F;
      case 1: n--;
        r[1] = ( r[1] & 0x0F ) | ( ( s[0] & 0x03 ) << 4 );
        r[0] = ( s[0] & 0xFC ) >> 2;
    };
    switch( m ){
      default: m--;
        o[3] = base64chars[ r[3] ];
      case 3: m--;
        o[2] = base64chars[ r[2] ];
      case 2: m--;
        o[1] = base64chars[ r[1] ];
      case 1: m--;
        o[0] = base64chars[ r[0] ];
    }
  }
}
