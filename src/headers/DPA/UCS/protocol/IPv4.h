#ifndef DPAUCS_IPv4_H
#define DPAUCS_IPv4_H

#include <DPA/UCS/protocol/address.h>

#define DPAUCS_ETH_T_IPv4 0x0800
#define DPAUCS_LA_IPv4( a,b,c,d ) DPAUCS_LA( DPAUCS_ETH_T_IPv4, 4, (a,b,c,d) )
#define DPAUCS_ADDR_IPv4( MAC, IP ) DPAUCS_ADDR( DPAUCS_ETH_T_IPv4, 4, MAC, IP )

DPA_MODULE( IPv4 );

#endif
