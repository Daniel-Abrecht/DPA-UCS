#ifndef DPAUCS_RESOURCE_H
#define DPAUCS_RESOURCE_H

#include <stdbool.h>
#include <DPA/utils/stream.h>
#include <DPA/utils/helper_macros.h>

#define DPAUCS_EXPORT_RESSOURCE_HANDLER( NAME, DRIVER ) \
  DPA_SECTION_LIST_ENTRY_HACK( const struct DPAUCS_ressource_handler*, DPAUCS_ressource_handler, DPAUCS_ressource_handler_ ## NAME ) DRIVER

#define DPAUCS_EACH_RESSOURCE_HANDLER( ITERATOR ) \
  DPA_FOR_SECTION_LIST_HACK( const struct DPAUCS_ressource_handler*, DPAUCS_ressource_handler, ITERATOR )

DPA_MODULE( ressource );

typedef struct DPAUCS_ressource_entry {
  const struct DPAUCS_ressource_handler* handler;
  const flash char* path;
  unsigned path_length;
} DPAUCS_ressource_entry_t;

typedef struct DPAUCS_ressource_handler {
  const DPAUCS_ressource_entry_t* (*open)( const char* path, unsigned length );
  void (*close)( const DPAUCS_ressource_entry_t* );
  size_t (*read)( const DPAUCS_ressource_entry_t*, DPA_stream_t* );
  const flash char* (*getMime)( const DPAUCS_ressource_entry_t* );
  const char* (*getHash)( const DPAUCS_ressource_entry_t* );
} DPAUCS_ressource_entry_handler_t;

const DPAUCS_ressource_entry_t* ressourceOpen( const char* path, unsigned length );

#endif
