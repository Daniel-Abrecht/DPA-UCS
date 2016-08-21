#include <avr/io.h>
#include <avr/interrupt.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/adelay.h>

#define PRESCALER        1024
#define PRESCALER_BITS   (1<<CS00)|(1<<CS02)
#define COUNTER_BITS        8

DPA_MODULE( adelay_driver ){}

static uint16_t time;

ISR(TIMER0_OVF_vect){
  time++;
  adelay_update_time(
    time, 0xFFFFu,
    PRESCALER * (1ul<<COUNTER_BITS) / ( F_CPU / 100ul )
  );
}

DPAUCS_INIT( adelay_timer ){
  TCCR0 = PRESCALER_BITS; 
  // Overflow Interrupt erlauben
  TIMSK |= (1<<TOIE0);
}
