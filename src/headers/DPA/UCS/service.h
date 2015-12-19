#ifndef SERVICE_H
#define SERVICE_H

#include <stddef.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>

typedef struct {
  uint8_t tos;
  void(*start)();
  bool(*onopen)( void* cid );
  void(*onreceive)( void* cid, void* data, size_t size );
  void(*oneof)( void* cid );
  void(*onclose)( void* cid );
  void(*stop)();
} DPAUCS_service_t;

#endif
