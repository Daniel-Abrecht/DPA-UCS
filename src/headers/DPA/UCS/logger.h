#ifndef DPAUCS_LOGGER
#define DPAUCS_LOGGER

#include <DPA/UCS/helper_macros.h>

#define DPAUCS_DEFINE_LOGGER( LOG_FUNC ) \
  int(*DPAUCS_log_func)(const char*,...) = LOG_FUNC;

#define DPAUCS_LOG( ... ) do { \
  if( DPAUCS_log_func ) \
    (*DPAUCS_log_func)( __FILE__ ":" DPAUCS_TOSTRING(__LINE__) " | " __VA_ARGS__ ); \
  } while(0)


extern int(*DPAUCS_log_func)(const char*,...);


#endif