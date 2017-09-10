#include <stdint.h>
#include <avr/io.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/spi.h>

uint8_t DPAUCS_spi_tranceive( uint8_t data ){
  SPDR = data;
  while(!( SPSR & (1<<SPIF) ));
  return SPDR;
}

DPAUCS_INIT {
  SPCR  =  (1<<SPE) | (1<<MSTR);
  SPSR |=  (1<<SPI2X);
  DDRB |=    (1<<5) | (1<<7);
  DDRB &= ~( (1<<2) | (1<<3) | (1<<4) | (1<<6) );
}

DPAUCS_SHUTDOWN {
  SPCR  = 0;
  SPSR &= ~(1<<SPI2X);
}
