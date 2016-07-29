#include <time.h>
#include <DPA/UCS/adelay.h>

DPAUCS_MODUL( adelay_driver ){}

void adelay_update( void ){
  static const unsigned long CLOCK_TYPE_MAX = (((1lu<<(sizeof(clock_t)*8u-1u))-1ul)<<1)+1lu;
  adelay_update_time( clock(), CLOCK_TYPE_MAX ? CLOCK_TYPE_MAX : ~0lu, CLOCKS_PER_SEC );
}
