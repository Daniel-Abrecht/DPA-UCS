#ifndef DPA_LOGGER_H
#define DPA_LOGGER_H

#include <DPA/UCS/helper_macros.h>

#ifdef DPA_LOGGER_C
#define DPA_WEAK_LOGGER
#else
#define DPA_WEAK_LOGGER weak
#endif

#define DPA_LOG( ... ) do { \
    if( DPA_log_func ) \
      (*DPA_log_func)( __FILE__ ":" DPA_TOSTRING(__LINE__) " | " __VA_ARGS__ ); \
  } while(0)

DPA_WEAK_LOGGER int (*const DPA_log_func)( const char*, ... );

#endif