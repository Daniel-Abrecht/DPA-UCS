#include <files.g1.h>
#include <DPA/utils/stream.h>
#include <DPA/UCS/files.h>

static const DPAUCS_ressource_entry_t* open( const char* path, unsigned length ){
  for( size_t i=0; i<files_size; i++ ){
    if( files[i].super.path_length != length )
      goto next;
    for( size_t j=0; j<length; j++ )
      if( path[j] != files[i].super.path[j] )
        goto next;
    return &files[i].super;
   next:;
  }
  return 0;
}

static void close( const DPAUCS_ressource_entry_t* e ){
  (void)e;
}

static size_t read( const DPAUCS_ressource_entry_t* e, DPA_stream_t* s ){
  DPAUCS_ressource_file_t* f = (DPAUCS_ressource_file_t*)e;
  DPA_stream_progmemWrite( s, f->content, f->size );
  return 0;
}

static const flash char* getMime( const DPAUCS_ressource_entry_t* e ){
  return ((DPAUCS_ressource_file_t*)e)->mime;
}

static const char* getHash( const DPAUCS_ressource_entry_t* e ){
  return ((DPAUCS_ressource_file_t*)e)->hash;
}

const flash DPAUCS_ressource_file_handler_t file_ressource_handler = {
  .super = {
    .open = open,
    .close = close,
    .read = read,
    .getMime = getMime,
    .getHash = getHash
  }
};

DPA_LOOSE_LIST_ADD( DPAUCS_ressource_handler_list, &file_ressource_handler.super )
