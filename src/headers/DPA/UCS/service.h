#ifndef DPAUCS_SERVICE_H
#define DPAUCS_SERVICE_H

#include <stddef.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>

typedef struct DPAUCS_service {
  uint8_t tos;
  void(*start)( void );
  bool(*onopen)( void* cid );
  void(*onreceive)( void* cid, void* data, size_t size );
  void(*oneof)( void* cid );
  void(*onclose)( void* cid );
  void(*stop)( void );
} DPAUCS_service_t;

#endif
