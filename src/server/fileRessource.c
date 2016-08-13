#include <stdio.h>
#include <string.h>
#include <DPA/UCS/files.h>
#include <files.g1.h>
#include <DPA/UCS/ressource.h>
#include <DPA/UCS/fileRessource.h>

#define N files_size
#define F files

DPA_MODULE( file_ressource ){
  DPA_DEPENDENCY( ressource );
}


const DPAUCS_ressource_file_t* getFileRessource( const char* path, unsigned length ){
  const flash DPAUCS_ressource_file_t *it, *end;
  for( it=F, end=F+N; it<end; it++ )
    if( length == it->entry.path_length
     && !memcmp( path, it->entry.path, length )
    ) return it;
  return 0;
}

bool DPAUCS_writeFileRessourceHeader( DPA_stream_t* stream, const DPAUCS_ressource_file_t* file ){
  (void)stream;
  (void)file;
  return true;
}

bool DPAUCS_writeFileRessource( DPA_stream_t* stream, const DPAUCS_ressource_file_t* file ){
  return DPA_stream_referenceWrite( stream, file->content, file->size );
}
