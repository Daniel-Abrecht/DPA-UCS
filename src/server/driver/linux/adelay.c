#include <sys/time.h>
#include <DPA/UCS/driver/adelay.h>

DPA_MODULE( adelay_driver ){}

void adelay_update( void ){
  struct timeval tv;
  gettimeofday(&tv,NULL);
  unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
  adelay_update_time( time_in_micros, ~0, 1000000 );
}
