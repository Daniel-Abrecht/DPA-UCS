#ifndef SERVICE_H
#define SERVICE_H

#include <stddef.h>

typedef struct {
  void(*start)();
  void(*onrecive)( uint16_t cd, void* data, size_t size );
  void(*stop)();
} DPAUCS_service_t;

#endif
