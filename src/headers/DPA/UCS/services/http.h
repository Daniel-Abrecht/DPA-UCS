#ifndef SERVICE_HTTP
#define SERVICE_HTTP

#include <DPA/UCS/service.h>

DPA_MODULE( http );

#ifdef HTTP_C
#define W
#else
#define W weak
#endif

extern DPAUCS_service_t weak http_service;

#undef W

#endif