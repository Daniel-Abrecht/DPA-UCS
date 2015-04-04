#ifndef FILES_H
#define FILES_H

#include <stddef.h>

#define URL_ENTRY \
  const entry_type type; \
  const char*const path;

typedef enum {
  FILE_ENTRY,
  FUNCTION_ENTRY
} entry_type;

typedef struct {
  URL_ENTRY
} entry_t;

typedef struct {
  URL_ENTRY
  const char*const mime;
  const size_t size;
  const char*const content;
} file_t;

typedef struct buffer_t buffer_t;
typedef struct http_t http_t;

typedef struct { 
  URL_ENTRY
  unsigned(*func)( const http_t*, buffer_t*const );
} function_t;

#endif
