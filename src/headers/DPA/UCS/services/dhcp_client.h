#ifndef DPAUCS_SERVICE_DHCP_CLIENT_H
#define DPAUCS_SERVICE_DHCP_CLIENT_H

#include <DPA/UCS/service.h>

DPA_MODULE( dhcp_client );

#ifndef DPAUCS_DHCP_CLIENT_MAX_LEASES
#define DPAUCS_DHCP_CLIENT_MAX_LEASES 3
#endif

extern const flash DPAUCS_service_t dhcp_client_service;

weak bool DPAUCS_is_interface_using_dhcp( DPAUCS_interface_t* interface );
void DPAUCS_dhcp_client_update_interface( DPAUCS_interface_t* interface );

#endif
