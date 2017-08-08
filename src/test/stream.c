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

MTest(stream,getLength){
  bool has_more;
  const char a[] = "Hello World\n";

  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),0,"wrong result 1");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,0,&has_more),0,"wrong result 2");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  cr_assert_eq(DPA_stream_getLength(&stream,0,0),0,"wrong result 3");

  DPA_stream_copyWrite(&stream,a,strlen(a));
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a),"wrong result 4");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a),"wrong result 5");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  cr_assert_eq(DPA_stream_getLength(&stream,~0,0),strlen(a),"wrong result 6");
  has_more = false;
  cr_assert_eq(DPA_stream_getLength(&stream,5,&has_more),5,"wrong result 7");
  cr_assert_eq(has_more,true,"has_more wasn't set to true");
  cr_assert_eq(DPA_stream_getLength(&stream,5,0),5,"wrong result 8");
  cr_assert_eq(DPA_stream_getLength(&stream,strlen(a),0),strlen(a),"wrong result 9");

  DPA_stream_referenceWrite(&stream,a,strlen(a));
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a)*2,"wrong result 10");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a)*2,"wrong result 11");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  cr_assert_eq(DPA_stream_getLength(&stream,~0,0),strlen(a)*2,"wrong result 12");
  has_more = false;
  cr_assert_eq(DPA_stream_getLength(&stream,5,&has_more),5,"wrong result 13");
  cr_assert_eq(has_more,true,"has_more wasn't set to true");
  cr_assert_eq(DPA_stream_getLength(&stream,5,0),5,"wrong result 14");
  cr_assert_eq(DPA_stream_getLength(&stream,strlen(a)*2,0),strlen(a)*2,"wrong result 15");

  DPA_stream_seek(&stream,strlen(a)+6);
  cr_assert_eq(ostBufferBuffer.range.offset,1,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,1,"Ringbuffer 2 content size should be 1");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.size,strlen(a),"Wrong stream entity size");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.offset,6,"Stream entity offset should be 6");
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a)-6,"wrong result 16");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  has_more = true;
  cr_assert_eq(DPA_stream_getLength(&stream,~0,&has_more),strlen(a)-6,"wrong result 17");
  cr_assert_eq(has_more,false,"has_more wasn't set to false");
  cr_assert_eq(DPA_stream_getLength(&stream,~0,0),strlen(a)-6,"wrong result 18");
  has_more = false;
  cr_assert_eq(DPA_stream_getLength(&stream,5,&has_more),5,"wrong result 19");
  cr_assert_eq(has_more,true,"has_more wasn't set to true");
  cr_assert_eq(DPA_stream_getLength(&stream,5,0),5,"wrong result 20");
  cr_assert_eq(DPA_stream_getLength(&stream,strlen(a)-6,0),strlen(a)-6,"wrong result 21");
}

MTest(stream,seek_empty){
  DPA_stream_seek(&stream,6);
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 0");
  cr_assert_eq(ostBufferBuffer.range.size,0,"Ringbuffer 2 content size should be 0");
}

MTest(stream,seek_skip_only_buffer_entry){
  const char a[] = "Hello World\n";
  DPA_stream_copyWrite(&stream,a,strlen(a));
  DPA_stream_seek(&stream,strlen(a));
  cr_assert_eq(ostBuffer.range.offset,strlen(a),"Ringbuffer 1 wrong offset");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,1,"Ringbuffer 2 offset should be 1");
  cr_assert_eq(ostBufferBuffer.range.size,0,"Ringbuffer 2 content size should be 0");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,strlen(a),"Wrong stream object offset");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,strlen(a),"Wrong stream object size");
}

MTest(stream,seek_skip_only_array_entry){
  const char a[] = "Hello World\n";
  DPA_stream_referenceWrite(&stream,a,strlen(a));
  DPA_stream_seek(&stream,strlen(a));
  cr_assert_eq(ostBuffer.range.offset,0,"Ringbuffer 1 offset should be 0");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 content size should be 0");
  cr_assert_eq(ostBufferBuffer.range.offset,1,"Ringbuffer 2 offset should be 1");
  cr_assert_eq(ostBufferBuffer.range.size,0,"Ringbuffer 2 content size should be 0");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,strlen(a),"Wrong stream object offset");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,strlen(a),"Wrong stream object size");
}

MTest(stream,seek_skip_partial){
  const char a[] = "Hello World\n";
  DPA_stream_copyWrite(&stream,a,strlen(a));
  DPA_stream_seek(&stream,6);
  cr_assert_eq(ostBuffer.range.offset,6,"Ringbuffer 1 wrong offset");
  cr_assert_eq(ostBuffer.range.size,strlen(a)-6,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,0,"Ringbuffer 2 offset should be 1");
  cr_assert_eq(ostBufferBuffer.range.size,1,"Ringbuffer 2 content size should be 0");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,6,"Wrong stream object offset");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,strlen(a),"Wrong stream object size");
}

MTest(stream,read){
  const char a[] = "Hello World\n";
  DPA_stream_copyWrite(&stream,a,strlen(a));
  DPA_stream_referenceWrite(&stream,a,strlen(a));
  char res[sizeof(a)*2-2];
  DPA_stream_read(&stream,res,strlen(a)*2);
  cr_assert_eq(ostBuffer.range.offset,strlen(a),"Ringbuffer 1 wrong offset");
  cr_assert_eq(ostBuffer.range.size,0,"Ringbuffer 1 wrong content size");
  cr_assert_eq(ostBufferBuffer.range.offset,2,"Ringbuffer 2 offset should be 2");
  cr_assert_eq(ostBufferBuffer.range.size,0,"Ringbuffer 2 content size should be 0");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.offset,strlen(a),"Wrong offset in stream object 0");
  cr_assert_eq(ostBufferBuffer.buffer[0].range.size,strlen(a),"Wrong size in stream object 0");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.offset,strlen(a),"Wrong offset in stream object 1");
  cr_assert_eq(ostBufferBuffer.buffer[1].range.size,strlen(a),"Wrong size in stream object 1");
  cr_assert(!memcmp(a,res,strlen(a)),"First part of content read doasn't match what was written");
  cr_assert(!memcmp(a,res+strlen(a),strlen(a)),"Second part of content read doasn't match what was written");
}
