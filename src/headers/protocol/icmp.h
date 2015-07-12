#ifndef ICMP_H
#define ICMP_H

#define PROTOCOL_ICMP 1

#include <helper_macros.h>

DPAUCS_MODUL( icmp );

void WEAK DPAUCS_icmpInit();
void WEAK DPAUCS_icmpShutdown();

#endif
