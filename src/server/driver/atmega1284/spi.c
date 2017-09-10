#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/spi.h>

uint8_t DPAUCS_spi_tranceive( uint8_t data ){
  SPDR = data;
  while(!( SPSR & (1<<SPIF) ));
  return SPDR;
}

DPAUCS_INIT {
  cli();
  SPCR  =  (1<<SPE) | (1<<SPIE) | (1<<MSTR);
  DDRB |=    (1<<5) | (1<<7);
  DDRB &= ~( (1<<2) | (1<<3) | (1<<4) | (1<<6) );
  sei();
}

DPAUCS_SHUTDOWN {
  cli();
  SPCR = 0;
  sei();
}
