#include <DPA/UCS/adelay.h>

static unsigned last;
static adelay_t time;

void adelay_update_time( unsigned long current, unsigned long max_mask, unsigned long ticks_per_sec ){
  unsigned passed = (unsigned long)( ( current - last ) & max_mask ) * AD_SEC / ticks_per_sec;
  if( !passed )
    return;
  last += (unsigned long)passed * ticks_per_sec / AD_SEC;
  time += passed;
}

void adelay_start( adelay_t* delay ){
  *delay = time;
}

bool adelay_done( adelay_t* delay, uint16_t duration ){
  return time-*delay >= duration;
}
