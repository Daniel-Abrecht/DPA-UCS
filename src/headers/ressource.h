#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>
#include <helper_macros.h>
#include <stream.h>


DPAUCS_MODUL( ressource );


typedef enum {
#ifdef RESSOURCE_GETTER
#define X( RG ) DPAUCS_RESSOURCE_ ## RG,
  RESSOURCE_GETTER
#undef X
#endif
  DPAUCS_RESSOURCE_COUNT
} DPAUCS_ressource_entry_type;

typedef struct {
  const DPAUCS_ressource_entry_type type;
  const char* path;
  unsigned path_length;
} DPAUCS_ressource_entry_t;

const DPAUCS_ressource_entry_t* defaultRessourceGetter( const char* path, unsigned length );
const DPAUCS_ressource_entry_t* WEAK getRessource( const char* path, unsigned length );
bool DPAUCS_defaultWriteRessourceHeaders( DPAUCS_stream_t*, const DPAUCS_ressource_entry_t* );
bool WEAK DPAUCS_writeRessourceHeaders( DPAUCS_stream_t*, const DPAUCS_ressource_entry_t* );
bool DPAUCS_defaultWriteRessource( DPAUCS_stream_t*, const DPAUCS_ressource_entry_t* );
bool WEAK DPAUCS_writeRessource( DPAUCS_stream_t*, const DPAUCS_ressource_entry_t* );

#endif
