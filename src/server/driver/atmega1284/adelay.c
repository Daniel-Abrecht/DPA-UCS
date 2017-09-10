#include <avr/io.h>
#include <avr/interrupt.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/adelay.h>

#define COUNTER_BITS        8
#define PRESCALER        1024
#define PRESCALER_BITS   (1<<CS00)|(1<<CS02)

DPA_MODULE( adelay_driver ){}

static uint16_t time;

ISR(TIMER0_OVF_vect){
  time++;
  adelay_update_time(
    time, 0xFFFFu,
    F_CPU / PRESCALER / (1ul<<COUNTER_BITS)
  );
}

DPAUCS_INIT {
  cli();
  TCCR0A = 0;
  TCCR0B = PRESCALER_BITS; 
  TIMSK0 = (1<<TOIE0);
  sei();
}

DPAUCS_SHUTDOWN {
  cli();
  TCCR0A = 0;
  TCCR0B = 0; 
  TIMSK0 = 0;
  sei();
}
