#ifndef DPAUCS_SERVICE_WEBSOCKET_H
#define DPAUCS_SERVICE_WEBSOCKET_H

#include <DPA/UCS/service.h>

DPA_MODULE( websocket );

extern const flash DPAUCS_service_t websocket_service;

typedef struct DPAUCS_websocket_subprotocol {
  const char* name;
  void(*onopen)( void* cid );
  void(*onreceive_utf8)( void* cid, void* data, size_t size, bool fin );
  void(*onreceive_binary)( void* cid, void* data, size_t size, bool fin );
  void(*onclose)( void* cid );
} DPAUCS_websocket_subprotocol_t;

DPA_LOOSE_LIST_DECLARE( const flash DPAUCS_websocket_subprotocol_t*, websocket_subprotocol_list )

#endif
