#include <stdio.h>
#include <services/http.h>
#include <protocol/tcp.h>

DPAUCS_MODUL( http ){
  DPAUCS_DEPENDENCY( tcp );
}

static int counter = 0;

static void start(){
  if(counter++) return;
  DPAUCS_tcpInit();
}

static void stop(){
  if(--counter) return;
  DPAUCS_tcpShutdown();
}

static void onreceive( void* cid, void* data, size_t size ){
  printf("http_service->onrecive: \n");
  fwrite( data, 1, size, stdout );
  (void)cid;
}

DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .start = start,
  .stop = stop,
  .onreceive = &onreceive
};
