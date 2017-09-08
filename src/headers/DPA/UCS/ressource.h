#ifndef DPAUCS_RESOURCE_H
#define DPAUCS_RESOURCE_H

#include <stdbool.h>
#include <DPA/utils/stream.h>
#include <DPA/utils/helper_macros.h>

DPA_MODULE( ressource );

typedef struct DPAUCS_ressource_entry {
  const flash struct DPAUCS_ressource_handler* handler;
  const flash char* path;
  const unsigned path_length;
} DPAUCS_ressource_entry_t;

typedef struct DPAUCS_ressource_handler {
  const DPAUCS_ressource_entry_t* (*open)( const char* path, unsigned length );
  void (*close)( const DPAUCS_ressource_entry_t* );
  size_t (*read)( const DPAUCS_ressource_entry_t*, DPA_stream_t* );
  const flash char* (*getMime)( const DPAUCS_ressource_entry_t* );
  const char* (*getHash)( const DPAUCS_ressource_entry_t* );
} DPAUCS_ressource_entry_handler_t;

DPA_LOOSE_LIST_DECLARE( const flash struct DPAUCS_ressource_handler*, DPAUCS_ressource_handler_list )

const DPAUCS_ressource_entry_t* ressourceOpen( const char* path, unsigned length );

#endif
