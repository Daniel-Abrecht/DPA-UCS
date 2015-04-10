#include <base_def.h>
#include <server.h>

#define MAC(X) X( 5c,f9,dd,55,96,c2 )

uint8_t __attribute__((weak)) mac[] = MAC(MACCONST);

int main(void){

  DPAUCS_init();

  DPAUCS_add_ip(IPINT(192,168,1,29));
  DPAUCS_add_ip(IPINT(192,168,1,39));

  while(1){
    DPAUCS_do_next_task();
  }

//  DPAUCS_shutdown();

}
