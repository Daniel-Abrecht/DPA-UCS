#ifndef DPAUCS_RESOURCE_H
#define DPAUCS_RESOURCE_H

#include <stdbool.h>
#include <DPA/UCS/stream.h>
#include <DPA/UCS/helper_macros.h>


DPA_MODULE( ressource );


enum DPAUCS_ressource_entry_type {
#ifdef RESSOURCE_GETTER
#define X( RG ) DPAUCS_RESSOURCE_ ## RG,
  RESSOURCE_GETTER
#undef X
#endif
  DPAUCS_RESSOURCE_COUNT
};

typedef struct DPAUCS_ressource_entry {
  const enum DPAUCS_ressource_entry_type type;
  const char* path;
  unsigned path_length;
} DPAUCS_ressource_entry_t;

const DPAUCS_ressource_entry_t* defaultRessourceGetter( const char* path, unsigned length );
const DPAUCS_ressource_entry_t* weak getRessource( const char* path, unsigned length );
bool DPAUCS_defaultWriteRessourceHeaders( DPA_stream_t*, const DPAUCS_ressource_entry_t* );
bool weak DPAUCS_writeRessourceHeaders( DPA_stream_t*, const DPAUCS_ressource_entry_t* );
bool DPAUCS_defaultWriteRessource( DPA_stream_t*, const DPAUCS_ressource_entry_t* );
bool weak DPAUCS_writeRessource( DPA_stream_t*, const DPAUCS_ressource_entry_t* );

#endif
