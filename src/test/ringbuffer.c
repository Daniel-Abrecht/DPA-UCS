#include <criterion/criterion.h>
#include <DPA/utils/ringbuffer.h>

/*
TEST FOR: utils/ringbuffer
DEPENDS: utils/ringbuffer
DEPENDS: utils/utils
*/

#define FTest(...) Test(__VA_ARGS__, .init = setup_normal)
#define RTest(...) Test(__VA_ARGS__, .init = setup_reverse)

DPA_RINGBUFFER_TEMPLATE( int, DPAUCS_test_ringbuffer );
DPA_DEFINE_RINGBUFFER( int, DPAUCS_test_ringbuffer_t, rb, 4, false );

/* setup */

static void setup_normal( void ){
  rb.inverse = false;
  DPA_ringbuffer_reset(&rb.super);
  cr_assert_eq(rb.size,4,"Ringbuffer size should be 4");
  cr_assert_eq(rb.range.offset,0,"Ringbuffer offset should be 0");
  cr_assert_eq(rb.range.size,0,"Ringbuffer content size should be 0");
  cr_assert(!rb.inverse,"Ringbuffer shouldn't be inversed");
  cr_assert_eq(rb.type_size,sizeof(int),"Ringbuffer entry size is wrong");
  cr_assert(rb.buffer,"Ringbuffer buffer must be set");
}

static void setup_reverse( void ){
  rb.inverse = true;
  DPA_ringbuffer_reset(&rb.super);
  cr_assert_eq(rb.size,4,"Ringbuffer size should be 4");
  cr_assert_eq(rb.range.offset,4,"Ringbuffer offset should be 4");
  cr_assert_eq(rb.range.size,0,"Ringbuffer content size should be 0");
  cr_assert(rb.inverse,"Ringbuffer should be inversed");
  cr_assert_eq(rb.type_size,sizeof(int),"Ringbuffer entry size is wrong");
  cr_assert(rb.buffer,"Ringbuffer buffer must be set");
}

/* DPA_ringbuffer_write */

FTest(ringbuffer,write){
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,257},2);
  cr_assert_eq(res,2,"Check if number of written bytes matches");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,2,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){1,257}), 2*sizeof(int) );
}

RTest(ringbuffer_reverse,write){
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,257},2);
  cr_assert_eq(res,2,"Check if number of written bytes matches");
  cr_assert_eq(rb.range.offset,4,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,2,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer+2, ((int[]){257,1}), 2*sizeof(int) );
}

FTest(ringbuffer,write_full){
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,2,3,4,5},5);
  cr_assert_eq(res,4,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,0,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){1,2,3,4}), 4*sizeof(int) );
}

RTest(ringbuffer_reverse,write_full){
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,2,3,4,5},5);
  cr_assert_eq(res,4,"Check if number of written bytes doesn't exceed buffer size");
  cr_assert_eq(rb.range.offset,4,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){4,3,2,1}), 4*sizeof(int) );
}

FTest(ringbuffer,write_overflow){
  rb.range.offset = 2;
  rb.buffer[1] = 4;
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,2,3},3);
  cr_assert_eq(res,3,"Check if only 3 elements have been written");
  cr_assert_eq(rb.range.offset,2,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){3,4,1,2}), 4*sizeof(int) );
}

RTest(ringbuffer_reverse,write_overflow){
  rb.range.offset = 2;
  rb.buffer[2] = 4;
  size_t res = DPA_ringbuffer_write(&rb.super,(int[]){1,2,3},3);
  cr_assert_eq(res,3,"Check if only 3 elements have been written");
  cr_assert_eq(rb.range.offset,2,"Check if offset hasn't chnged");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_arr_eq( rb.buffer, ((int[]){2,1,4,3}), 4*sizeof(int) );
}

/* DPA_ringbuffer_read */

FTest(ringbuffer,read){
  int destination[4] = {0};
  int expected[] = {256,257,258,259};
  memcpy( rb.buffer, expected, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_arr_eq( destination, expected, 3*sizeof(int) );
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,read){
  int destination[4] = {0};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){259,258,257,256}), 3*sizeof(int) );
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,read_full){
  int destination[] = {0,0,0,0,10};
  int expected[] = {256,257,258,259,10};
  memcpy( rb.buffer, expected, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, expected, 5*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,read_too_much){
  int destination[] = {0,0,0,0,10};
  int expected[] = {256,257,258,259,10};
  memcpy( rb.buffer, expected, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 5 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_arr_eq( destination, expected, 5*sizeof(int) );
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

RTest(ringbuffer_reverse,read_full){
  int destination[] = {0,0,0,0,10};
  memcpy( rb.buffer, (int[]){256,257,258,259}, 4*sizeof(int) );
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 4 );
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
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 3 );
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
  size_t ret = DPA_ringbuffer_read( &rb.super, destination, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_arr_eq( destination, ((int[]){257,256,259,10}), 4*sizeof(int) );
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

/* DPA_ringbuffer_skip_read */

FTest(ringbuffer,skip_read){
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,skip_read){
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,skip_read_full){
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,skip_read_too_much){
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 5 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

RTest(ringbuffer_reverse,skip_read_full){
  rb.range.size = 4;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,0,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

FTest(ringbuffer,skip_read_overflow){
  rb.range.size = 4;
  rb.range.offset = 2;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,skip_read_overflow){
  rb.range.size = 4;
  rb.range.offset = 2;
  size_t ret = DPA_ringbuffer_skip_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,1,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

/* DPA_ringbuffer_rewind_read */

FTest(ringbuffer,rewind_read){
  rb.range.offset = 3;
  rb.range.size = 0;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

RTest(ringbuffer_reverse,rewind_read){
  rb.range.offset = 1;
  rb.range.size = 0;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

FTest(ringbuffer,rewind_read_full){
  rb.range.offset = 3;
  rb.range.size = 0;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

FTest(ringbuffer,rewind_read_too_much){
  rb.range.offset = 3;
  rb.range.size = 0;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 5 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,rewind_read_full){
  rb.range.offset = 1;
  rb.range.size = 0;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 4 );
  cr_assert_eq(ret,4,"wrong amount read");
  cr_assert_eq(rb.range.size,4,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,rewind_read_overflow){
  rb.range.size = 0;
  rb.range.offset = 2;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,rewind_read_overflow){
  rb.range.size = 0;
  rb.range.offset = 2;
  size_t ret = DPA_ringbuffer_rewind_read( &rb.super, 3 );
  cr_assert_eq(ret,3,"wrong amount read");
  cr_assert_eq(rb.range.size,3,"Check if size was correctly updated");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

/* DPA_ringbuffer_reverse */
FTest(ringbuffer,reverse){
  rb.range.size = 2;
  rb.range.offset = 1;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,2,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse){
  rb.range.size = 2;
  rb.range.offset = 3;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,2,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,reverse_full){
  rb.range.size = 4;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,4,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_full){
  rb.range.size = 4;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,4,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,reverse_empty){
  rb.range.offset = 1;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_empty){
  rb.range.offset = 1;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

FTest(ringbuffer,reverse_wrap_empty){
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,4,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_wrap_empty){
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,0,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,0,"Check if offset is correct");
}

FTest(ringbuffer,reverse_overflow){
  rb.range.size = 3;
  rb.range.offset = 2;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,3,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,1,"Check if offset is correct");
}

RTest(ringbuffer_reverse,reverse_overflow){
  rb.range.offset = 2;
  rb.range.size = 3;
  DPA_ringbuffer_reverse(&rb.super);
  cr_assert_eq(rb.range.size,3,"Check if size hasn't changed");
  cr_assert_eq(rb.range.offset,3,"Check if offset is correct");
}

/* DPA_ringbuffer_increment_write */

FTest(ringbuffer,increment_write){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {0,1,0},
    {1,2,0},
    {2,3,0},
    {3,4,0},
    {3,4,0}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

RTest(ringbuffer_reverse,increment_write){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {3,1,4},
    {2,2,4},
    {1,3,4},
    {0,4,4},
    {0,4,4}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

FTest(ringbuffer,increment_write_overflow){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {3,1,3},
    {0,2,3}
  };
  rb.range.offset = 3;
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

RTest(ringbuffer_reverse,increment_write_overflow){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {0,1,1},
    {3,2,1}
  };
  rb.range.offset = 1;
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}



/* DPA_ringbuffer_decrement_write */

FTest(ringbuffer,decrement_write){
  rb.range.size = 4;
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {3,3,0},
    {2,2,0},
    {1,1,0},
    {0,0,0},
    {0,0,0}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_decrement_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

RTest(ringbuffer_reverse,decrement_write){
  rb.range.size = 4;
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {0,3,4},
    {1,2,4},
    {2,1,4},
    {3,0,4},
    {3,0,4}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_decrement_write(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

/* DPA_ringbuffer_increment_read */

FTest(ringbuffer,increment_read){
  rb.range.size = 4;
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {0,3,1},
    {1,2,2},
    {2,1,3},
    {3,0,0},
    {3,0,0}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_read(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

RTest(ringbuffer_reverse,increment_read){
  rb.range.size = 4;
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {3,3,3},
    {2,2,2},
    {1,1,1},
    {0,0,4},
    {0,0,4}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_increment_read(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

/* DPA_ringbuffer_decrement_read */

FTest(ringbuffer,decrement_read){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {3,1,3},
    {2,2,2},
    {1,3,1},
    {0,4,0},
    {0,4,0}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_decrement_read(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}

RTest(ringbuffer_reverse,decrement_read){
  struct params {
    size_t ret, size, offset;
  };
  struct params results[] = {
    {0,1,1},
    {1,2,2},
    {2,3,3},
    {3,4,4},
    {3,4,4}
  };
  for(
    struct params *it=results, *end=results+sizeof(results)/sizeof(*results);
    it < end; it++
  ){
    size_t ret = DPA_ringbuffer_decrement_read(&rb.super);
    cr_assert_eq(ret,it->ret,"Check if returned offset is correct");
    cr_assert_eq(rb.range.size,it->size,"Check if size is correct");
    cr_assert_eq(rb.range.offset,it->offset,"Check if offset hasn't changed");
  }
}
