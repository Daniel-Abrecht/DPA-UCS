#include <DPA/UCS/ressource.h>

DPA_MODULE( ressource ){}

const DPAUCS_ressource_entry_t* ressourceOpen( const char* path, unsigned length ){
  for( struct DPAUCS_ressource_handler_list* it = DPAUCS_ressource_handler_list; it; it = it->next ){
    const DPAUCS_ressource_entry_t* entry = it->entry->open( path, length );
    if( entry )
      return entry;
  }
  return 0;
}
