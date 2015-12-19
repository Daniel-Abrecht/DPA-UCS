#ifndef ASYNC_DELAY_H
#define ASYNC_DELAY_H

#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>

DPAUCS_MODUL( adelay_driver );

typedef uint32_t adelay_t;

void adelay_start( adelay_t* ); // starts the delay
// Checks if the delay finished, duration 1/100s
bool adelay_done( adelay_t*, uint16_t );
void WEAK adelay_update( void );
void adelay_update_time( unsigned current_ticks, unsigned long ticks_max, unsigned ticks_per_sec );

#endif