#include <base_def.h>
#include <server.h>

#define MAC(X) X( 60,36,DD,26,4A,BC )

const uint8_t mac[] = MAC(MACCONST);

int main(void){

  DPAUCS_init();

  DPAUCS_add_ip(IPINT(192,168,1,29));
  DPAUCS_add_ip(IPINT(192,168,1,39));

  while(1){
    DPAUCS_do_next_task();
  }

  DPAUCS_shutdown();

}
