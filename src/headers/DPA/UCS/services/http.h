#ifndef DPAUCS_SERVICE_HTTP_H
#define DPAUCS_SERVICE_HTTP_H

#include <DPA/UCS/service.h>
#include <DPA/UCS/ressource.h>
#include <DPA/utils/stream.h>
#include <DPA/utils/helper_macros.h>

DPA_MODULE( http );

extern const flash DPAUCS_service_t http_service;

typedef struct DPAUCS_http_upgrade_handler {
  const flash char* protocol;
  const flash DPAUCS_service_t* service;
  bool (*start)( void* cid );
  bool (*process_header)(
    void* cid,
    size_t key_length, const char key[key_length],
    size_t value_length, const char value[value_length]
  );
  bool (*add_response_headers)( void* cid, DPA_stream_t* stream );
  const void* (*getssdata)( void* cid );
  void (*abort)( void* cid );
} DPAUCS_http_upgrade_handler_t;

DPA_LOOSE_LIST_DECLARE( const flash DPAUCS_http_upgrade_handler_t*, http_upgrade_handler_list )

#endif
