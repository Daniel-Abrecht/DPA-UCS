#include <DPA/UCS/driver/eth/driver.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/services/http.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/address.h>

static const DPAUCS_interface_t* getFirstInterface(){

  DPAUCS_EACH_ETHERNET_DRIVER(it){
    if( it->driver->interface_count )
      return it->driver->interfaces;
  }

  return 0;
}

void server_main(void* arg){
  (void)arg;

  const DPAUCS_interface_t* interface = getFirstInterface();

#ifdef USE_IPv4
  static const DPAUCS_logicAddress_IPv4_t IPv4addrs[] = {
    DPAUCS_LA_IPv4(192,168,1,29),
    DPAUCS_LA_IPv4(192,168,1,39),
    DPAUCS_LA_IPv4(192,168,43,29),
    DPAUCS_LA_IPv4(192,168,8,29)
  };

  for( unsigned i=0; i<sizeof(IPv4addrs); i++ )
    DPAUCS_add_logicAddress( interface, &IPv4addrs[i].logicAddress );
#endif

  DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 80, &http_service );

  while(1){
    DPAUCS_doNextTask();
  }

}

int main(void){

  DPAUCS_run(server_main,0);

  return 0;
}
