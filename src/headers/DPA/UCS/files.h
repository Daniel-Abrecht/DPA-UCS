#ifndef DPAUCS_FILES_H
#define DPAUCS_FILES_H

#include <stddef.h>
#include <DPA/UCS/ressource.h>

typedef struct DPAUCS_ressource_file {
  DPAUCS_ressource_entry_t entry;
  const flash char* mime;
  size_t size;
  const flash char* content;
} DPAUCS_ressource_file_t;

#endif
