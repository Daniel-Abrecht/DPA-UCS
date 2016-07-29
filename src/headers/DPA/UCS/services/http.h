#ifndef SERVICE_HTTP
#define SERVICE_HTTP

#include <DPA/UCS/service.h>

DPAUCS_MODUL( http );

#ifdef HTTP_C
#define W
#else
#define W WEAK
#endif

extern DPAUCS_service_t WEAK http_service;

#undef W

#endif