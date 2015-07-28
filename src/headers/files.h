#ifndef FILES_H
#define FILES_H

#include <stddef.h>

#ifdef __FLASH
#define DP_FLASH //__flash
#else
#define DP_FLASH
#endif

typedef enum {
  FILE_ENTRY,
  FUNCTION_ENTRY
} entry_type;

typedef struct {
  const entry_type type;
  DP_FLASH const char* path;
} entry_t;

typedef struct {
  entry_t entry;
  const DP_FLASH char* mime;
  size_t size;
  const DP_FLASH char* content;
} file_t;

#endif
