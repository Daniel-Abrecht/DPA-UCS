#include <DPA/UCS/ressource.h>

DPA_MODULE( ressource ){}

const DPAUCS_ressource_entry_t* defaultRessourceGetter( const char* path, unsigned length ){

  (void)path;
  (void)length;

  DPAUCS_ressource_entry_t* ressource = 0;

#ifdef RESSOURCE_GETTER
#define X( RG ) DPAUCS_ressource_entry_t* get ## RG(const char*,unsigned);
  RESSOURCE_GETTER
#undef X
#endif

#ifdef RESSOURCE_GETTER
#define X( RG ) if(!( ressource = get ## RG(path,length) ))
  RESSOURCE_GETTER{}
#undef X
#endif

  return ressource;
}


const DPAUCS_ressource_entry_t* weak getRessource( const char* path, unsigned length ){
  return defaultRessourceGetter( path, length );
}

bool DPAUCS_defaultWriteRessourceHeaders( DPA_stream_t* stream, const DPAUCS_ressource_entry_t* ressource ){
#ifdef RESSOURCE_GETTER
#define X( RG ) bool DPAUCS_write ## RG ## Header(DPA_stream_t*,const DPAUCS_ressource_entry_t*);
  RESSOURCE_GETTER
#undef X
#endif

#ifdef RESSOURCE_GETTER
#define X( RG ) case DPAUCS_RESSOURCE_ ## RG: return DPAUCS_write ## RG ## Header(stream,ressource);
switch( ressource->type ){
  RESSOURCE_GETTER
  default:;
}
#undef X
#endif
  return true;
}

bool weak DPAUCS_writeRessourceHeaders( DPA_stream_t* stream, const DPAUCS_ressource_entry_t* ressource ){
  return DPAUCS_defaultWriteRessourceHeaders( stream, ressource );
}

bool weak DPAUCS_defaultWriteRessource( DPA_stream_t* stream, const DPAUCS_ressource_entry_t* ressource ){
#ifdef RESSOURCE_GETTER
#define X( RG ) bool DPAUCS_write ## RG(DPA_stream_t*,const DPAUCS_ressource_entry_t*);
  RESSOURCE_GETTER
#undef X
#endif

#ifdef RESSOURCE_GETTER
#define X( RG ) case DPAUCS_RESSOURCE_ ## RG: return DPAUCS_write ## RG(stream,ressource);
switch( ressource->type ){
  RESSOURCE_GETTER
  default:;
}
#undef X
#endif
  return true;
}

bool weak DPAUCS_writeRessource( DPA_stream_t* stream, const DPAUCS_ressource_entry_t* ressource ){
  return DPAUCS_defaultWriteRessource( stream, ressource );
}

