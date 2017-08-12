#include <DPA/UCS/ressource.h>

DPA_MODULE( ressource ){}

const DPAUCS_ressource_entry_t* ressourceOpen( const char* path, unsigned length ){
  DPAUCS_EACH_RESSOURCE_HANDLER( ressource_handler ){
    const DPAUCS_ressource_entry_t* entry = (*ressource_handler)->open( path, length );
    if( entry )
      return entry;
  }
  return 0;
}
