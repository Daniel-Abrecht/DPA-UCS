#ifndef SERVICE_H
#define SERVICE_H

#include <stddef.h>
#include <stdint.h>
#include <helper_macros.h>

typedef struct {
  uint8_t tos; // type of service
  void(*start)();
  void(*onrecive)( uint16_t cd, void* data, size_t size );
  void(*stop)();
} DPAUCS_service_t;

#endif
