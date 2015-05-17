#include <base_def.h>
#include <server.h>
#include <protocol/ip.h>
#include <services/http.h>
#include <protocol/icmp.h>

#define MAC(X) X( 5c,f9,dd,55,96,c2 )

uint8_t __attribute__((weak)) mac[] = MAC(MACCONST);

int main(void){

  DPAUCS_init();

  DPAUCS_add_ip(IPINT(192,168,1,29));
  DPAUCS_add_ip(IPINT(192,168,43,29));

  DPAUCS_icmpInit();
  DPAUCS_add_service( IP_ANY, 80, &http_service );

  while(1){
    DPAUCS_doNextTask();
  }

//  DPAUCS_shutdown();

}
