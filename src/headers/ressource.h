#ifndef RESOURCE_H
#define RESOURCE_H

#include <helper_macros.h>

DPAUCS_MODUL( ressource );


typedef enum {
  DPAUCS_RESSOURCE_FILE_ENTRY,
  DPAUCS_RESSOURCE_FUNCTION_ENTRY
} DPAUCS_ressource_entry_type;

typedef struct {
  const DPAUCS_ressource_entry_type type;
  DP_FLASH const char* path;
} DPAUCS_ressource_entry_t;


DPAUCS_ressource_entry_t* defaultRessourceGetter( const char* path, unsigned length );
DPAUCS_ressource_entry_t* WEAK getRessource( const char* path, unsigned length );

#endif
