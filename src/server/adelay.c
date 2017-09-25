#include <DPA/UCS/driver/adelay.h>
#include <DPA/UCS/server.h>

static volatile adelay_t time;

void adelay_update_time( unsigned long current, unsigned long max_mask, unsigned long ticks_per_sec ){
  static unsigned long last;
  unsigned long passed = (unsigned long)( ( current - last ) & max_mask ) * AD_SEC / ticks_per_sec;
  if( !passed )
    return;
  last += (unsigned long)passed * ticks_per_sec / AD_SEC;
  time += passed;
}

void adelay_start( adelay_t* delay ){
  *delay = time;
  if(!*delay)
    *delay = 1;
}

bool adelay_done( adelay_t* delay, adelay_duration_t duration ){
  if( !*delay )
    return true;
  if( adelay_update )
    adelay_update();
  return time-*delay >= duration;
}

void adelay_wait( adelay_duration_t duration ){
  adelay_t x;
  adelay_start( &x );
  while( !adelay_done( &x, duration ) );
}

DPAUCS_TASK {
  if( adelay_update )
    adelay_update();
}
