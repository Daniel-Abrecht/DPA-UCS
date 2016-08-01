#include <criterion/criterion.h>
#include <DPA/UCS/ringbuffer.h>

#define FTest(...) Test(__VA_ARGS__, .init = setup_normal)
#define RTest(...) Test(__VA_ARGS__, .init = setup_reverse)

DPAUCS_RINGBUFFER_TEMPLATE( int, DPAUCS_test_ringbuffer );
DPAUCS_DEFINE_RINGBUFFER( int, DPAUCS_test_ringbuffer_t, rb, 4, false );

/* setup */

void setup_normal( void ){
  rb.inverse = false;
  DPAUCS_ringbuffer_reset(&rb.super);
  cr_assert_eq(rb.size,4,"Ringbuffer size should be 4");
  cr_assert_eq(rb.range.offset,0,"Ringbuffer offset should be 0");
  cr_assert_eq(rb.range.size,0,"Ringbuffer content size should be 0");
  cr_assert(!rb.inverse,"Ringbuffer shouldn't be inversed");
  cr_assert_eq(rb.type_size,sizeof(int),"Ringbuffer entry size is wrong");
  cr_assert(rb.buffer,"Ringbuffer buffer must be set");
}

void setup_reverse( void ){
  rb.inverse = true;
  DPAUCS_ringbuffer_reset(&rb.super);
  cr_assert_eq(rb.size,4,"Ringbuffer size should be 4");
  cr_assert_eq(rb.range.offset,4,"Ringbuffer offset should be 4");
  cr_assert_eq(rb.range.size,0,"Ringbuffer content size should be 0");
  cr_assert(rb.inverse,"Ringbuffer should be inversed");
  cr_assert_eq(rb.type_size,sizeof(int),"Ringbuffer entry size is wrong");
  cr_assert(rb.buffer,"Ringbuffer buffer must be set");
}

/* DPAUCS_ringbuffer_write */

FTest(ringbuffer,write){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,257},2);
  cr_assert_eq(res,2,"Check if number of written bytes matches");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,2,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){1,257}), 2*sizeof(int) );
}

RTest(ringbuffer_reverse,write){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,257},2);
  cr_assert_eq(res,2,"Check if number of written bytes matches");
  cr_assert_eq(rb.range.offset,4,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,2,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer+2, ((int[]){257,1}), 2*sizeof(int) );
}

FTest(ringbuffer,write_full){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3,4,5},5);
  cr_assert_eq(res,4,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){1,2,3,4}), 4*sizeof(int) );
}

RTest(ringbuffer_reverse,write_full){
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3,4,5},5);
  cr_assert_eq(res,4,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,4,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){4,3,2,1}), 4*sizeof(int) );
}

FTest(ringbuffer,write_overflow){
  rb.range.offset = 2;
  rb.buffer[1] = 4;
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3},3);
  cr_assert_eq(res,3,"Check if only 3 elements have been written");
  cr_assert_eq(rb.range.offset,2,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){3,4,1,2}), 4*sizeof(int) );
}

RTest(ringbuffer_reverse,write_overflow){
  rb.range.offset = 2;
  rb.buffer[2] = 4;
  size_t res = DPAUCS_ringbuffer_write(&rb.super,(int[]){1,2,3},3);
  cr_assert_eq(res,3,"Check if only 3 elements have been written");
  cr_assert_eq(rb.range.offset,2,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){2,1,4,3}), 4*sizeof(int) );
}

/* DPAUCS_ringbuffer_read */

FTest(ringbuffer,read){
  int destination[4] = {0};
  int expected[] = {256,257,258,259};
  memcpy( rb.buffer, expected, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, expected, 4*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

RTest(ringbuffer_reverse,read){
  int destination[4] = {0};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){259,258,257,256}), 4*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

FTest(ringbuffer,read_full){
  int destination[] = {0,0,0,0,10};
  int expected[] = {256,257,258,259,10};
  memcpy( rb.buffer, expected, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, expected, 5*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

RTest(ringbuffer_reverse,read_full){
  int destination[] = {0,0,0,0,10};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){259,258,257,256,10}), 5*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

FTest(ringbuffer,read_overflow){
  int destination[] = {0,0,0,10};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  rb.range.offset = 2;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){258,259,256,10}), 4*sizeof(int) );
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,read_overflow){
  int destination[4] = {0,0,0,10};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  rb.range.offset = 2;
  size_t ret = DPAUCS_ringbuffer_read( &rb.super, destination, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){257,256,259,10}), 4*sizeof(int) );
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

/* DPAUCS_ringbuffer_reverse */
FTest(ringbuffer,reverse){
  rb.range.size = 2;
  rb.range.offset = 1;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,2,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse){
  rb.range.size = 2;
  rb.range.offset = 3;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,2,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,reverse_full){
  rb.range.size = 4;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,4,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_full){
  rb.range.size = 4;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,4,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,reverse_empty){
  rb.range.offset = 1;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_empty){
  rb.range.offset = 1;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,reverse_wrap_empty){
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_wrap_empty){
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,reverse_overflow){
  rb.range.size = 3;
  rb.range.offset = 2;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,3,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_overflow){
  rb.range.offset = 2;
  rb.range.size = 3;
  DPAUCS_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,3,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

FTest(ringbuffer,increment_write){
  DPAUCS_ringbuffer_increment_write(&rb.super);
  cr_assert_eq(rb.range.size,1,"Check if size is correct");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't changed");
}

FTest(ringbuffer,increment_write_full){
  rb.range.size = 4;
  DPAUCS_ringbuffer_increment_write(&rb.super);
  cr_assert_eq(rb.range.size,4,"Check if size is correct");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't changed");
}
