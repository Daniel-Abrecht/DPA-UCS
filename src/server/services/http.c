#include <stdio.h>
#include <services/http.h>

DPAUCS_MODUL( http ){}

static bool onopen( void* cid ){
  printf("http_service->onopen\n");
  (void)cid;
  return true;
}

static void onreceive( void* cid, void* data, size_t size ){
  printf("http_service->onrecive: \n");
  fwrite( data, 1, size, stdout );
  (void)cid;
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
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .onclose = &onclose
};
