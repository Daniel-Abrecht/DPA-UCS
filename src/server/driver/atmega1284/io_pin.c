#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/io_pin.h>

#define REG_BIT( PIN ) (1<<((PIN)&7))
#define REG_PORT( REGS, PIN ) *((REGS)[(PIN)>>3])
#define PBIT_ON(  REGS, PIN )   REG_PORT( REGS, PIN ) |=  REG_BIT(PIN)
#define PBIT_OFF( REGS, PIN )   REG_PORT( REGS, PIN ) &= ~REG_BIT(PIN)
#define PBIT_GET( REGS, PIN ) ( REG_PORT( REGS, PIN )  &  REG_BIT(PIN) )

#define ASSERT_PIN( PIN ) if(PIN>=(sizeof(ports)/sizeof(*ports))*8) DPAUCS_fatal(pin_out_of_range_message)
static const flash char pin_out_of_range_message[] = {"IO pin out of range\n"};

static volatile uint8_t*const flash ports[] = {
  &PORTA, &PORTB, &PORTC, &PORTD
};

static volatile uint8_t*const flash ddrs[] = {
  &DDRA, &DDRB, &DDRC, &DDRD
};

static volatile uint8_t*const flash pins[] = {
  &PINA, &PINB, &PINC, &PIND
};

void DPAUCS_io_set_pin_mode( unsigned pin, DPAUCS_io_pin_mode_t mode ){
  ASSERT_PIN(pin);
  if( mode ){
    PBIT_ON (ddrs,pin);
  }else{
    PBIT_OFF(ddrs,pin);
  }
}

void DPAUCS_io_set_pin( unsigned pin, bool state, uint16_t delay_us_t_10 ){
  ASSERT_PIN(pin);
  if( state ){
    PBIT_ON (ports,pin);
  }else{
    PBIT_OFF(ports,pin);
  }
  while( delay_us_t_10-- )
    _delay_us(10);
}

bool DPAUCS_io_get_pin( unsigned pin ){
  ASSERT_PIN(pin);
  return PBIT_GET(pins,pin);
}
