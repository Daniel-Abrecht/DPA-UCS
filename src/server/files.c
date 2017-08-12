#include <string.h>
#include <files.g1.h>
#include <DPA/utils/stream.h>
#include <DPA/UCS/files.h>

static const DPAUCS_ressource_entry_t* open( const char* path, unsigned length ){
  for( size_t i=0; i<files_size; i++ ){
    if( files[i].super.path_length == length
     && !memcmp( files[i].super.path, path, length )
    ) return &files[i].super;
  }
  return 0;
}

static void close( const DPAUCS_ressource_entry_t* e ){
  (void)e;
}

static size_t read( const DPAUCS_ressource_entry_t* e, DPA_stream_t* s ){
  DPAUCS_ressource_file_t* f = (DPAUCS_ressource_file_t*)e;
  DPA_stream_referenceWrite( s, f->content, f->size );
  return 0;
}

static const char* getMime( const DPAUCS_ressource_entry_t* e ){
  return ((DPAUCS_ressource_file_t*)e)->mime;
}

static const char* getHash( const DPAUCS_ressource_entry_t* e ){
  return ((DPAUCS_ressource_file_t*)e)->hash;
}

DPAUCS_ressource_file_handler_t file_ressource_handler = {
  .super = {
    .open = open,
    .close = close,
    .read = read,
    .getMime = getMime,
    .getHash = getHash
  }
};

DPAUCS_EXPORT_RESSOURCE_HANDLER( file, &file_ressource_handler.super );