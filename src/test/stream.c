#include <criterion/criterion.h>
#include <DPA/utils/stream.h>

#define MTest(...) Test(__VA_ARGS__, .init = setup)

// TEST FOR: utils/stream

DPA_DEFINE_RINGBUFFER(unsigned char,DPA_uchar_ringbuffer_t,ostBuffer,256,false);
DPA_DEFINE_RINGBUFFER(DPA_bufferInfo_t,DPA_buffer_ringbuffer_t,ostBufferBuffer,10,false);

static DPA_stream_t stream = {
  &ostBuffer,
  &ostBufferBuffer
};

static void setup( void ){
  DPA_stream_reset(&stream);
  cr_assert_eq(ostBuffer.size,256,"Ringbuffer 1 size should be 256");
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 content size should be 0");
  cr_assert(!ostBuffer.inverse,"Ringbuffer 1 shouldn't be inversed");
  cr_assert_eq(ostBuffer.type_size,sizeof(unsigned char),"Ringbuffer 1 entry size is wrong");
  cr_assert(ostBuffer.buffer,"Ringbuffer 1 buffer must be set");
  cr_assert_eq(ostBufferBuffer.size,10,"Ringbuffer 2 size should be 10");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,0,"Ringbuffer 2 content size should be 0");
  cr_assert(!ostBufferBuffer.inverse,"Ringbuffer 2 shouldn't be inversed");
  cr_assert_eq(ostBufferBuffer.type_size,sizeof(DPA_bufferInfo_t),"Ringbuffer 2 entry size is wrong");
  cr_assert(ostBufferBuffer.buffer,"Ringbuffer 2 buffer must be set");
}

MTest(stream,referenceWrite){
  const char a[] = "Hello World\n";
  DPA_stream_referenceWrite(&stream,a,sizeof(a)-1);
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 content size should be 0");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,1,"Ringbuffer 2 content size should be 1");
  cr_assert_eq(ostBufferBuffer.buffer[0].type,BUFFER_ARRAY,"Stream entry 0 has wrong type");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,0,"Stream entry 0 has wrong offset");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,sizeof(a)-1,"Stream entry 0 has wrong size");
  cr_assert_eq(ostBufferBuffer.buffer[0].ptr,a,"Stream entry 0 points to the wrong thing");
  const char b[] = "Hello World\n";
  DPA_stream_referenceWrite(&stream,b,sizeof(b)-1);
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 content size should be 0");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,2,"Ringbuffer 2 content size should be 1");
  cr_assert_eq(ostBufferBuffer.buffer[1].type,BUFFER_ARRAY,"Stream entry 0 has wrong type");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.offset,0,"Stream entry 0 has wrong offset");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.size,sizeof(b)-1,"Stream entry 0 has wrong size");
  cr_assert_eq(ostBufferBuffer.buffer[1].ptr,b,"Stream entry 0 points to the wrong thing");
}

MTest(stream,copyWrite){
  const char a[] = "Hello World\n";
  DPA_stream_copyWrite(&stream,a,sizeof(a)-1);
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,sizeof(a)-1,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,1,"Ringbuffer 2 content size should be 1");
  cr_assert_eq(ostBufferBuffer.buffer[0].type,BUFFER_BUFFER,"Stream entry 0 has wrong type");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,0,"Stream entry 0 has wrong offset");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,sizeof(a)-1,"Stream entry 0 has wrong size");
  cr_assert(!ostBufferBuffer.buffer[0].ptr,"Stream entry pointer should be null");
  const char b[] = "test\n";
  DPA_stream_copyWrite(&stream,b,sizeof(b)-1);
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,sizeof(a)+sizeof(b)-2,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,2,"Ringbuffer 2 content size should be 1");
  cr_assert_eq(ostBufferBuffer.buffer[1].type,BUFFER_BUFFER,"Stream entry 0 has wrong type");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.offset,0,"Stream entry 0 has wrong offset");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.size,sizeof(b)-1,"Stream entry 0 has wrong size");
  cr_assert(!ostBufferBuffer.buffer[1].ptr,"Stream entry pointer should be null");
  cr_assert(!memcmp( ostBuffer.buffer, a, sizeof(a)-1 ), "First string wasn't stored correctly" );
  cr_assert(!memcmp( ostBuffer.buffer+sizeof(a)-1, b, sizeof(b)-1 ), "Second string wasn't stored correctly" );
}
