#ifndef SERVICE_DHCP_H
#define SERVICE_DHCP_H

#include <DPA/UCS/service.h>

DPA_MODULE( dhcp );

extern const flash DPAUCS_service_t dhcp_service;

bool DPAUCS_dhcp_add( DPAUCS_interface_t* interface );
bool DPAUCS_dhcp_remove( DPAUCS_interface_t* interface );

#endif
