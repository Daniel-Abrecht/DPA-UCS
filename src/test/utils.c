#include <criterion/criterion.h>
#include <DPA/utils/utils.h>

// TEST FOR: server/utils

Test(utils,DPA_memrcpy){
  int src[] = {1,2,3,4};
  int res[4] = {0};
  DPA_memrcpy( sizeof(int), res, src, 4 );
  cr_assert_arr_eq( res, ((int[]){4,3,2,1}), 4 );
}

Test(utils,DPA_mempos_check_has_result){
  size_t position;
  char haystack[] = "__test__test__";
  char needle[] = "test";
  bool res = DPA_mempos( &position, haystack, sizeof(haystack)-1, needle, sizeof(needle)-1 );
  cr_assert(res,"needle probably not found in haystack");
  cr_assert_eq(position,2,"wrong position returned");
}

Test(utils,DPA_mempos_check_has_no_result){
  size_t position = 77;
  char haystack[] = "__";
  char needle[] = "test";
  bool res = DPA_mempos( &position, haystack, sizeof(haystack)-1, needle, sizeof(needle)-1 );
  cr_assert(!res,"needle found in haystack, but doesn't exist in haystack");
  cr_assert_eq(position,77,"position was changed");
}

Test(utils,DPA_mempos_check_no_needle){
  size_t position = 77;
  char haystack[] = "__";
  bool res = DPA_mempos( &position, haystack, sizeof(haystack)-1, 0, 0 );
  cr_assert(!res,"needle found in haystack, but doesn't exist in haystack");
  cr_assert_eq(position,77,"position was changed");
}

Test(utils,DPA_memrcharpos_check_has_result){
  size_t position = 77;
  char haystack[] = "1234";
  bool res = DPA_memrcharpos( &position, sizeof(haystack)-1, haystack, '3' );
  cr_assert(res,"needle probably not found in haystack");
  cr_assert_eq(position,2,"wrong position returned");
}

Test(utils,DPA_memrcharpos_check_has_no_result){
  size_t position = 77;
  char haystack[] = "1234";
  bool res = DPA_memrcharpos( &position, sizeof(haystack)-1, haystack, '5' );
  cr_assert(!res,"needle found in haystack, but doesn't exist in haystack");
  cr_assert_eq(position,77,"position was changed");
}

Test(utils,DPA_memtrim_content){
  const char string[] = "***test**";
  const char* start = string;
  size_t size = sizeof(string)-1;
  DPA_memtrim( &start, &size, '*' );
  cr_assert_eq(start,string+3,"wrong offset");
  cr_assert_eq(size,4,"wrong size");
}

Test(utils,DPA_memtrim_filler){
  const char string[] = "****";
  const char* start = string;
  size_t size = sizeof(string)-1;
  DPA_memtrim( &start, &size, '*' );
  cr_assert_eq(size,0,"wrong size");
}

Test(utils,DPA_memtrim_empty){
  const char string[] = "";
  const char* start = string;
  size_t size = sizeof(string)-1;
  DPA_memtrim( &start, &size, '*' );
  cr_assert_eq(size,0,"wrong size");
}

Test(utils,DPA_streq_nocase_with_equal_strings){
  cr_assert( DPA_streq_nocase( "tEsT", "TEst", 4 ) );
}

Test(utils,DPA_streq_nocase_with_non_equal_strings){
  cr_assert( !DPA_streq_nocase( "tEsT1", "TEst2", 5 ) );
}
