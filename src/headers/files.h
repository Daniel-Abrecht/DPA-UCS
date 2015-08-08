#ifndef FILES_H
#define FILES_H

#include <stddef.h>

#ifdef __FLASH
#define DP_FLASH //__flash
#else
#define DP_FLASH
#endif

typedef enum {
  DPAUCS_RESOURCE_FILE_ENTRY,
  DPAUCS_RESOURCE_FUNCTION_ENTRY
} DPAUCS_resource_entry_type;

typedef struct {
  const DPAUCS_resource_entry_type type;
  DP_FLASH const char* path;
} DPAUCS_resource_entry_t;

typedef struct {
  DPAUCS_resource_entry_t entry;
  const DP_FLASH char* mime;
  size_t size;
  const DP_FLASH char* content;
} DPAUCS_resource_file_t;

#endif
