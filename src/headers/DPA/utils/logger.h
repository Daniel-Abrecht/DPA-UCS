#ifndef DPA_LOGGER_H
#define DPA_LOGGER_H

#include <DPA/utils/helper_macros.h>

#ifdef DPA_LOGGER_C
#define DPA_WEAK_LOGGER
#else
#define DPA_WEAK_LOGGER weak
#endif

#ifdef NO_LOGGING
#define DPA_LOG( ... ) do {} while(0)
#else
#ifndef __FLASH
#define DPA_LOG( ... ) do { \
    if( DPA_log_func ) \
      (*DPA_log_func)( __FILE__ ":" DPA_TOSTRING(__LINE__) " | " __VA_ARGS__ ); \
  } while(0)
#else
#define DPA_LOG( ... ) DPA_LOG_HELPER(__VA_ARGS__,"")
#define DPA_LOG_HELPER( M, ... ) do { \
    static const flash char format[] = { __FILE__ ":" DPA_TOSTRING(__LINE__) " | " M "%.0s" }; \
    if( DPA_log_func ) \
       (*DPA_log_func)( format, __VA_ARGS__ ); \
  } while(0)
#endif
#endif

#if defined(__FLASH)
extern DPA_WEAK_LOGGER int (*const DPA_log_func)( const flash char*, ... );
#elif defined(__GNUC__) && !defined(__STRICT_ANSI__)
extern DPA_WEAK_LOGGER int (*const DPA_log_func)( const flash char*, ... ) __attribute__ ((format (gnu_printf, 1, 2)));
#else
extern DPA_WEAK_LOGGER int (*const DPA_log_func)( const flash char*, ... ) __attribute__ ((format (printf, 1, 2)));
#endif

#endif