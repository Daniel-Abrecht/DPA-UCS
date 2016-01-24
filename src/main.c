#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/services/http.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>

uint8_t WEAK mac[] = { 0x5C, 0xF9, 0xDD, 0x55, 0x96, 0xC2 };

void server_main(void* arg){
  (void)arg;

#ifdef USE_IPv4
  static const DPAUCS_logicAddress_IPv4_t IPv4addrs[] = {
    DPAUCS_LA_IPv4(192,168,1,29),
    DPAUCS_LA_IPv4(192,168,1,39),
    DPAUCS_LA_IPv4(192,168,43,29),
    DPAUCS_LA_IPv4(192,168,8,29)
  };

  for( unsigned i=0; i<sizeof(IPv4addrs); i++ )
    DPAUCS_add_logicAddress( &IPv4addrs[i].logicAddress );
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
