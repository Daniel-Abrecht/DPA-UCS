#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <ressource.h>

typedef struct {
  DPAUCS_ressource_entry_t entry;
  const DP_FLASH char* mime;
  size_t size;
  const DP_FLASH char* content;
} DPAUCS_ressource_file_t;

#endif
