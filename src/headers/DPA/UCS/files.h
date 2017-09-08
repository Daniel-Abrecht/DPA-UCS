#ifndef DPAUCS_FILES_H
#define DPAUCS_FILES_H

#include <stddef.h>
#include <DPA/UCS/ressource.h>

typedef struct DPAUCS_ressource_file_handler {
  struct DPAUCS_ressource_handler super;
} DPAUCS_ressource_file_handler_t;

typedef struct DPAUCS_ressource_file {
  DPAUCS_ressource_entry_t super;
  const flash char* mime;
  const char* hash;
  size_t size;
  const flash char* content;
} DPAUCS_ressource_file_t;

extern const flash DPAUCS_ressource_file_handler_t file_ressource_handler;

#endif
