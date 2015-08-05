#include <stdio.h>
#include <protocol/tcp.h>
#include <services/http.h>

DPAUCS_MODUL( http ){
  DPAUCS_DEPENDENCY( tcp );
}

static bool onopen( void* cid ){
  printf("http_service->onopen\n");
  (void)cid;
  return true;
}

bool testfunc( DPAUCS_stream_t* stream ){
  static char ch[] = "HTTP/1.0 200 OK\r\n\r\n<h1>Hello World!</h1>";
  DPAUCS_stream_referenceWrite(stream,ch,sizeof(ch));
  return true;
}

static void onreceive( void* cid, void* data, size_t size ){
  printf("http_service->onrecive: \n");
  fwrite( data, 1, size, stdout );
  (void)cid;
  DPAUCS_tcp_send( testfunc, &cid, 1 );
}

static void oneof( void* cid ){
  printf("http_service->oneof\n");
  (void)cid;
}

static void onclose( void* cid ){
  printf("http_service->onclose\n");
  (void)cid;
}

DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .onclose = &onclose
};
