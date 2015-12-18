#include <server.h>
#include <protocol/address.h>
#include <protocol/ip.h>
#include <services/http.h>
#include <helper_macros.h>

#define MAC(X) X( 5c,f9,dd,55,96,c2 )

uint8_t WEAK mac[] = MAC(MACCONST);

void server_main(void* arg){
  (void)arg;

#ifdef USE_IPv4
  static const DPAUCS_logicAddress_IPv4_t IPv4addrs[] = {
    LA_IPv4(192,168,1,29),
    LA_IPv4(192,168,1,39),
    LA_IPv4(192,168,43,29),
    LA_IPv4(192,168,8,29)
  };

  for( unsigned i=0; i<sizeof(IPv4addrs); i++ )
    DPAUCS_add_logicAddress( &IPv4addrs[i].logicAddress );
#endif

  DPAUCS_add_service( ANY_ADDRESS, 80, &http_service );

  while(1){
    DPAUCS_doNextTask();
  }

}

int main(void){

  DPAUCS_run(server_main,0);

  return 0;
}
