#ifndef FILE_RESSOURCE_H
#define FILE_RESSOURCE_H

#include <helper_macros.h>


DPAUCS_MODUL( file_ressource );


const DPAUCS_ressource_file_t* getFileRessource( const char* path, unsigned length );
bool DPAUCS_writeFileRessourceHeader( DPAUCS_stream_t* stream, const DPAUCS_ressource_file_t* ressource );
bool DPAUCS_writeFileRessource( DPAUCS_stream_t* stream, const DPAUCS_ressource_file_t* ressource );


#endif