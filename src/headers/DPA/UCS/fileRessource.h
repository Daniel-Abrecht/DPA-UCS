#ifndef DPAUCS_FILE_RESSOURCE_H
#define DPAUCS_FILE_RESSOURCE_H

#include <DPA/utils/helper_macros.h>


DPA_MODULE( file_ressource );


const DPAUCS_ressource_file_t* getFileRessource( const char* path, unsigned length );
bool DPAUCS_writeFileRessourceHeader( DPA_stream_t* stream, const DPAUCS_ressource_file_t* ressource );
bool DPAUCS_writeFileRessource( DPA_stream_t* stream, const DPAUCS_ressource_file_t* ressource );


#endif