#include <criterion/criterion.h>
#include <DPA/utils/encoding/base64.h>

/*
TEST FOR: utils/encoding/base64
DEPENDS: utils/encoding/base64
*/

#define S(X) X, sizeof(X)-1

Test(encoding_base64,encode_reminder_0){
  const char in[] = "123";
  char out[10] = {0};
  DPA_encoding_base64_encode( out, sizeof(out), in, sizeof(in)-1 );
  const char expected[10] = "MTIz\0\0\0\0\0\0";
  cr_assert_arr_eq( out, expected, 10 );
}

Test(encoding_base64,encode_reminder_1){
  const char in[] = "1234";
  char out[10] = {0};
  DPA_encoding_base64_encode( out, sizeof(out), in, sizeof(in)-1 );
  const char expected[10] = "MTIzNA==\0\0";
  cr_assert_arr_eq( out, expected, 10 );
}

Test(encoding_base64,encode_reminder_2){
  const char in[] = "12345";
  char out[10] = {0};
  DPA_encoding_base64_encode( out, sizeof(out), in, sizeof(in)-1 );
  const char expected[10] = "MTIzNDU=\0\0";
  cr_assert_arr_eq( out, expected, 10 );
}


