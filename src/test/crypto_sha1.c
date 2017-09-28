#include <criterion/criterion.h>
#include <DPA/utils/crypto/sha1.h>

// TEST FOR: utils/crypto/sha1

#define S(X) X, sizeof(X)-1

Test(utils,sha1_empty){
  DPA_crypto_sha1_state_t sha1;
  DPA_crypto_sha1_init( &sha1 );
  uint8_t res[20];
  DPA_crypto_sha1_done( &sha1, res );
  uint8_t expected[] = {
    0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
    0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
  };
  cr_assert_arr_eq( res, expected, 20 );
}

Test(utils,sha1_small_one_piece){
  DPA_crypto_sha1_state_t sha1;
  DPA_crypto_sha1_init( &sha1 );
  DPA_crypto_sha1_add( &sha1, S("The quick brown fox jumps over the lazy dog") );
  uint8_t res[20];
  DPA_crypto_sha1_done( &sha1, res );
  uint8_t expected[] = {
    0x2f, 0xd4, 0xe1, 0xc6, 0x7a, 0x2d, 0x28, 0xfc, 0xed, 0x84,
    0x9e, 0xe1, 0xbb, 0x76, 0xe7, 0x39, 0x1b, 0x93, 0xeb, 0x12
  };
  cr_assert_arr_eq( res, expected, 20 );
}

Test(utils,sha1_multiple_pieces){
  DPA_crypto_sha1_state_t sha1;
  DPA_crypto_sha1_init( &sha1 );
  DPA_crypto_sha1_add( &sha1, S("The quick brown fox ") );
  DPA_crypto_sha1_add( &sha1, S("jumps over the lazy dog") );
  uint8_t res[20];
  DPA_crypto_sha1_done( &sha1, res );
  uint8_t expected[] = {
    0x2f, 0xd4, 0xe1, 0xc6, 0x7a, 0x2d, 0x28, 0xfc, 0xed, 0x84,
    0x9e, 0xe1, 0xbb, 0x76, 0xe7, 0x39, 0x1b, 0x93, 0xeb, 0x12
  };
  cr_assert_arr_eq( res, expected, 20 );
}

Test(utils,sha1_large_text){
  DPA_crypto_sha1_state_t sha1;
  DPA_crypto_sha1_init( &sha1 );
  DPA_crypto_sha1_add( &sha1, S(
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
    "The quick brown fox jumps over the lazy dog\n"
  ) );
  uint8_t res[20];
  DPA_crypto_sha1_done( &sha1, res );
  uint8_t expected[] = {
    0x8b, 0x21, 0xb7, 0x90, 0x13, 0x00, 0xf2, 0x57, 0x97, 0x5e,
    0x40, 0xbd, 0x0b, 0x39, 0xf9, 0x93, 0x29, 0xdc, 0x43, 0xa5
  };
  cr_assert_arr_eq( res, expected, 20 );
}

