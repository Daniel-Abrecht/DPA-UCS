#include <avr/io.h>
#include <avr/interrupt.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/adelay.h>

#define COUNTER_BITS        8
#define PRESCALER        1024
#define PRESCALER_BITS   (1<<CS00)|(1<<CS02)

DPA_MODULE( adelay_driver ){}

static uint16_t time;

ISR(TIMER0_OVF_vect){
  time++;
  adelay_update_time(
    time, 0xFFFFu,
    PRESCALER * (1ul<<COUNTER_BITS) / ( F_CPU / 100ul )
  );
}

DPAUCS_INIT {
  TCCR0B = PRESCALER_BITS; 
  // Overflow Interrupt erlauben
  TIMSK0 |= (1<<TOIE0);
}
