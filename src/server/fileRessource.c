#include <stdio.h>
#include <string.h>
#include <DPA/UCS/files.h>
#include <files.g1.h>
#include <DPA/UCS/ressource.h>
#include <DPA/UCS/fileRessource.h>

#define N files_size
#define F files

DPAUCS_MODUL( file_ressource ){
  DPAUCS_DEPENDENCY( ressource );
}


const DPAUCS_ressource_file_t* getFileRessource( const char* path, unsigned length ){
  const DP_FLASH DPAUCS_ressource_file_t *it, *end;
  for( it=F, end=F+N; it<end; it++ )
    if( length == it->entry.path_length
     && !memcmp( path, it->entry.path, length )
    ) return it;
  return 0;
}

bool DPAUCS_writeFileRessourceHeader( DPAUCS_stream_t* stream, const DPAUCS_ressource_file_t* file ){
  (void)stream;
  (void)file;
  return true;
}

bool DPAUCS_writeFileRessource( DPAUCS_stream_t* stream, const DPAUCS_ressource_file_t* file ){
  return DPAUCS_stream_referenceWrite( stream, file->content, file->size );
}
