#include <criterion/criterion.h>
#include <DPA/UCS/ringbuffer.h>

#define RTest(...) Test(__VA_ARGS__, .init = setup)

DPAUCS_RINGBUFFER_TEMPLATE( int, DPAUCS_test_ringbuffer );
DPAUCS_DEFINE_RINGBUFFER( int, DPAUCS_test_ringbuffer_t, rb, 4, false );

void setup( void ){
  rb.inverse = false;
  DPAUCS_ringbuffer_reset(&rb.super);
  cr_assert_eq(rb.size,4,"Ringbuffer size should be 4");
  cr_assert_eq(rb.range.offset,0,"Ringbuffer offset should be 0");
  cr_assert_eq(rb.range.size,0,"Ringbuffer content size should be 0");
  cr_assert(!rb.inverse,"Ringbuffer shouldn't be inversed");
  cr_assert_eq(rb.type_size,sizeof(int),"Ringbuffer entry size is wrong");
  cr_assert(rb.buffer,"Ringbuffer buffer must be set");
}

RTest(ringbuffer,write){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,257},2);
  cr_assert_eq(res,2,"Check if number of written bytes matches");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,2,"Check if size was correctly updated");
}

RTest(ringbuffer,write_full){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3,4,5},5);
  cr_assert_eq(res,4,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
}

RTest(ringbuffer,write_overflow){
  rb.range.offset = 2;
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3},3);
  cr_assert_eq(res,3,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,2,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
}
