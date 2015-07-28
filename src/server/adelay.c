#include <adelay.h>

static unsigned last, current;
static adelay_t time;

void WEAK adelay_update( void ){}

void adelay_update_time( unsigned current_ticks, unsigned long ticks_max, unsigned ticks_per_sec ){
  current = current_ticks;
  unsigned passed = (unsigned long)( ( ( current % ticks_max ) - ( last % ticks_max ) ) % ticks_max ) * 100u / ticks_per_sec;
  if( !passed )
    return;
  last += (unsigned long)passed * ticks_per_sec / 100u;
  time += passed;
}

void adelay_start( adelay_t* delay ){
  *delay = time;
}

bool adelay_done( adelay_t* delay, uint16_t duration ){
  return time-*delay > duration;
}
