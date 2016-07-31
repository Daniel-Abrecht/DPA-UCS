#include <criterion/criterion.h>
#include <DPA/UCS/utils.h>

Test(utils,memrcpy){
  int src[] = {1,2,3,4};
  int res[4] = {0};
  memrcpy( sizeof(int), res, src, 4 );
  cr_assert_arr_eq( res, ((int[]){4,3,2,1}), 4 );
}
