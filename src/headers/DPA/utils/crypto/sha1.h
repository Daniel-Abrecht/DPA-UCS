#ifndef DPA_CRYPTO_SHA1_H
#define DPA_CRYPTO_SHA1_H

#include <stdint.h>

typedef struct DPA_crypto_sha1_state {
  uint32_t hash[5];
  uint32_t window[16];
  uint64_t length;
} DPA_crypto_sha1_state_t;

void DPA_crypto_sha1_init( DPA_crypto_sha1_state_t* sha1 );
void DPA_crypto_sha1_add( DPA_crypto_sha1_state_t*restrict sha1, void* data, uint64_t n );
void DPA_crypto_sha1_done( DPA_crypto_sha1_state_t*restrict sha1, uint8_t result[20] );

#endif
