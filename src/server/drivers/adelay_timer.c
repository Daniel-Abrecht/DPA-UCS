#include <avr/io.h>
#include <avr/interrupt.h>
#include <adelay.h>

#define PRESCALER        1024
#define PRESCALER_BITS   (1<<CS00)|(1<<CS02)
#define COUNTER_BITS        8

DPAUCS_MODUL( adelay_driver ){}

static uint16_t time;

ISR(TIMER0_OVF_vect){
  time++;
  adelay_update_time(
    time,
    1<<sizeof(time),
    PRESCALER * (1ul<<COUNTER_BITS) / ( F_CPU / 100ul )
  );
}

void adelay_timer_init( void ){
  TCCR0 = PRESCALER_BITS; 
  // Overflow Interrupt erlauben
  TIMSK |= (1<<TOIE0);
}
