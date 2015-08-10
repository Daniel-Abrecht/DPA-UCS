#include <ressource.h>

DPAUCS_MODUL( ressource ){}

DPAUCS_ressource_entry_t* defaultRessourceGetter( const char* path, unsigned length ){

  (void)path;
  (void)length;

  DPAUCS_ressource_entry_t* ressource = 0;

#ifdef RESSOURCE_GETTER
#define X( RG ) if(!( resource = RG(path,length) ))
  RESSOURCE_GETTER;
#undef X
#endif

  return ressource;
}


DPAUCS_ressource_entry_t* WEAK getRessource( const char* path, unsigned length ){
  return defaultRessourceGetter( path, length );
}
