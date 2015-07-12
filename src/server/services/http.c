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

static void onrecive( uint16_t cd, void* data, size_t size ){
  (void)cd;
  (void)data;
  (void)size;
}

DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .start = start,
  .stop = stop,
  .onrecive = onrecive
};
