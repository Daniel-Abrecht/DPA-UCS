#ifndef DPAUCS_ICMP_H
#define DPAUCS_ICMP_H

#define PROTOCOL_ICMP 1

#include <DPA/UCS/helper_macros.h>

DPA_MODULE( icmp );

#ifdef DPAUCS_ICMP_C
#define W
#else
#define W weak
#endif

void W DPAUCS_icmpInit( void );
void W DPAUCS_icmpShutdown( void );

#undef W

#endif
