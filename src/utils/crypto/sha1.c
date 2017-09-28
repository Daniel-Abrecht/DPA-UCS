#include <stddef.h>
#include <string.h>
#include <DPA/utils/crypto/sha1.h>

#define RT_LEFT(x,n)  (((x) << n) | ((x) >> (32-n)))

void DPA_crypto_sha1_init( DPA_crypto_sha1_state_t* sha1 ){
  sha1->hash[0] = 0x67452301;
  sha1->hash[1] = 0xEFCDAB89;
  sha1->hash[2] = 0x98BADCFE;
  sha1->hash[3] = 0x10325476;
  sha1->hash[4] = 0xC3D2E1F0;
  sha1->length = 0;
}

#define W(X) *( X < 16 ? sha1->window + X : X - 16 + temp_win )
static void mix_it( DPA_crypto_sha1_state_t*restrict sha1 ){
  
  uint32_t temp_win[64] = {0};
  for(unsigned i=16;i<80;i++)
    W(i) = RT_LEFT( W(i-3) ^ W(i-8) ^ W(i-14) ^ W(i-16), 1 );

  uint32_t k, f, t[5];
  memcpy( t, sha1->hash, sizeof(t) );

  for( unsigned i=0; i<80; i++ ){
    if( i<20 ){
      f = ( t[1] & t[2] ) | ( (~t[1]) & t[3] );
      k = 0x5A827999;
    }else if( i < 40 ){
      f = t[1] ^ t[2] ^ t[3];
      k = 0x6ED9EBA1;
    }else if( i < 60 ){
      f = ( t[1] & t[2] ) | ( t[1] & t[3] ) | ( t[2] & t[3] ); 
      k = 0x8F1BBCDC;
    }else{
      f = t[1] ^ t[2] ^ t[3];
      k = 0xCA62C1D6;
    }

    uint32_t temp = RT_LEFT(t[0],5) + f + t[4] + k + W(i);
    t[4] = t[3];
    t[3] = t[2];
    t[2] = RT_LEFT(t[1], 30);
    t[1] = t[0];
    t[0] = temp;
  }

  for( unsigned i=5; i--; )
    sha1->hash[i] += t[i];

}
#undef W

#define X(Y,Z) \
void Y( DPA_crypto_sha1_state_t*restrict sha1, Z void* data, uint64_t n ){ \
  \
  Z uint8_t* in = data; \
  uint8_t shift = sha1->length % 64; \
  \
  for( ; n; n = n>64 ? n-64 : 0 ){ \
  \
    size_t i; \
    for( i=shift; i<(n+shift) && i<64; i++ ){ \
      if(!( i % 4 )){ \
        sha1->window[i/4] = (uint32_t)*(in++) << 24; \
      }else{ \
        sha1->window[i/4] |= (uint32_t)*(in++) << (24-(i%4)*8); \
      } \
    } \
  \
    sha1->length += i - shift; \
  \
    if( i == 64 ) \
      mix_it(sha1); \
  \
  } \
}
X(DPA_crypto_sha1_add,const)
#ifdef __FLASH
X(DPA_crypto_sha1_add_flash,const flash)
#endif
#undef X

void DPA_crypto_sha1_done( DPA_crypto_sha1_state_t*restrict sha1, uint8_t result[20] ){

  uint64_t bitlen = sha1->length * 8;

  DPA_crypto_sha1_add(sha1,(uint8_t[]){0x80},1);

  while( sha1->length % 64 != 56 )
    DPA_crypto_sha1_add( sha1, (uint8_t[]){0}, 1 );

  DPA_crypto_sha1_add( sha1, (uint8_t[]){
    ( bitlen >> 56 ) & 0xFF,
    ( bitlen >> 48 ) & 0xFF,
    ( bitlen >> 40 ) & 0xFF,
    ( bitlen >> 32 ) & 0xFF,
    ( bitlen >> 24 ) & 0xFF,
    ( bitlen >> 16 ) & 0xFF,
    ( bitlen >>  8 ) & 0xFF,
    ( bitlen >>  0 ) & 0xFF
  }, 8 );

  for( size_t i=0; i<20; i++ )
    *(result++) = ( sha1->hash[i/4] >> (24-(i%4)*8) ) & 0xFF;

}
