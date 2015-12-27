#ifndef HELPER_MACROS_H
#define HELPER_MACROS_H

#if __STDC_VERSION__ < 201112L
#pragma anon_unions
#ifndef noreturn
#define noreturn __attribute__((noreturn))
#endif
#else
#include <stdnoreturn.h>
#endif

#if __alignof_is_defined
#include <stdalign.h>
#define DPAUCS_ALIGNOF(T) alignof(T)
#else
#include <stddef.h>
#define DPAUCS_ALIGNOF(T) offsetof(struct{char x;T y;},y)
#endif

#define PACKED1
#define PACKED2 __attribute__ ((__packed__))

#define WEAK __attribute__((weak))
#define NORETURN noreturn

#define DPAUCS_MODUL( name ) void modul_ ## name( void )
#define DPAUCS_DEPENDENCY( name ) modul_ ## name()

// workarround for gcc bug 50925 in gcc < 4.9
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50925#c19
#if defined( __GNUC__ ) && ( __GNUC__ < 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ < 9 ) ) && defined( __OPTIMIZE_SIZE__ )
#define GCC_BUGFIX_50925 __attribute__((optimize("O3")))
#else
#define GCC_BUGFIX_50925
#endif

#ifdef __FLASH
#define DP_FLASH //__flash
#else
#define DP_FLASH
#endif

#define DPAUCS_CALC_ALIGN_OFFSET( x, T ) \
  ( (size_t)( (size_t)(x) + DPAUCS_ALIGNOF(T) - 1 ) & (size_t)~( DPAUCS_ALIGNOF(T) - 1 ) )

#endif
