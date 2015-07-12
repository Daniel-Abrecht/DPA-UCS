#ifndef HELPER_MACROS_H
#define HELPER_MACROS_H

#if __STDC_VERSION__ < 201112L
#pragma anon_unions
#endif

#define PACKED1
#define PACKED2 __attribute__ ((__packed__))

#define WEAK __attribute__((weak))

#define DPAUCS_MODUL( name ) void modul_ ## name( void )
#define DPAUCS_DEPENDENCY( name ) modul_ ## name()

#endif
