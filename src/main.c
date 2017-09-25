#include <DPA/UCS/driver/ethernet.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/services/http.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/address.h>

weak void server_main(void* arg){
  (void)arg;

  DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 80, &http_service, 0 );

  while(true)
    DPAUCS_doNextTask();

}

weak int main(void){
  DPAUCS_run(server_main,0);
  return 0;
}
