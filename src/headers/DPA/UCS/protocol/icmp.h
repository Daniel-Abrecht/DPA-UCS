#ifndef ICMP_H
#define ICMP_H

#define PROTOCOL_ICMP 1

#include <DPA/UCS/helper_macros.h>

DPAUCS_MODUL( icmp );

#ifdef ICMP_C
#define W
#else
#define W WEAK
#endif

void W DPAUCS_icmpInit( void );
void W DPAUCS_icmpShutdown( void );

#undef W

#endif
