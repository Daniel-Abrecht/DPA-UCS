#include <criterion/criterion.h>
#include <DPA/UCS/utils.h>

Test(utils,memrcpy){
  int src[] = {1,2,3,4};
  int res[4] = {0};
  memrcpy( sizeof(int), res, src, 4 );
  cr_assert_arr_eq( res, ((int[]){4,3,2,1}), 4 );
}

Test(utils,mempos_check_has_result){
  size_t position;
  char haystack[] = "__test__test__";
  char needle[] = "test";
  bool res = mempos( &position, haystack, sizeof(haystack)-1, needle, sizeof(needle)-1 );
  cr_assert(res,"needle probably not found in haystack");
  cr_assert_eq(position,2,"wrong position returned");
}

Test(utils,mempos_check_has_no_result){
  size_t position = 77;
  char haystack[] = "__";
  char needle[] = "test";
  bool res = mempos( &position, haystack, sizeof(haystack)-1, needle, sizeof(needle)-1 );
  cr_assert(!res,"needle found in haystack, but doesn't exist in haystack");
  cr_assert_eq(position,77,"position was changed");
}

Test(utils,memrcharpos_check_has_result){
  size_t position = 77;
  char haystack[] = "1234";
  bool res = memrcharpos( &position, sizeof(haystack)-1, haystack, '3' );
  cr_assert(res,"needle probably not found in haystack");
  cr_assert_eq(position,2,"wrong position returned");
}

Test(utils,memrcharpos_check_has_no_result){
  size_t position = 77;
  char haystack[] = "1234";
  bool res = memrcharpos( &position, sizeof(haystack)-1, haystack, '5' );
  cr_assert(!res,"needle found in haystack, but doesn't exist in haystack");
  cr_assert_eq(position,77,"position was changed");
}

Test(utils,memtrim_content){
  const char string[] = "***test**";
  const char* start = string;
  size_t size = sizeof(string)-1;
  memtrim( &start, &size, '*' );
  cr_assert_eq(start,string+3,"wrong offset");
  cr_assert_eq(size,4,"wrong size");
}

Test(utils,memtrim_filler){
  const char string[] = "****";
  const char* start = string;
  size_t size = sizeof(string)-1;
  memtrim( &start, &size, '*' );
  cr_assert_eq(size,0,"wrong size");
}

Test(utils,memtrim_empty){
  const char string[] = "";
  const char* start = string;
  size_t size = sizeof(string)-1;
  memtrim( &start, &size, '*' );
  cr_assert_eq(size,0,"wrong size");
}

Test(utils,streq_nocase_with_equal_strings){
  cr_assert( streq_nocase( "tEsT", "TEst", 4 ) );
}

Test(utils,streq_nocase_with_non_equal_strings){
  cr_assert( !streq_nocase( "tEsT1", "TEst2", 5 ) );
}
