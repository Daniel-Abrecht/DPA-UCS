#include <DPA/UCS/adelay.h>
#include <time.h>

DPAUCS_MODUL( adelay_driver ){}

void adelay_update( void ){
  adelay_update_time( clock(), 1<<sizeof(clock_t), CLOCKS_PER_SEC );
}
