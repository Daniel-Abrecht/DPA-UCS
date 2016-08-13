#ifndef DPAUCS_ASYNC_DELAY_H
#define DPAUCS_ASYNC_DELAY_H

#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>

DPA_MODULE( adelay_driver );

#define AD_SEC ((adelay_duration_t)100)

typedef uint32_t adelay_t;
typedef uint16_t adelay_duration_t;

void adelay_start( adelay_t* ); // starts the delay
// Checks if the delay finished, duration 1/AD_SEC
bool adelay_done( adelay_t*, adelay_duration_t );
void adelay_update_time( unsigned long current, unsigned long max_mask, unsigned long ticks_per_sec );

void weak adelay_update( void );

#endif