#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/spi.h>

static bool initialized = false;

void spi_init(){
  SPCR  =  (1<<SPE) | (1<<MSTR);
  SPSR |=  (1<<SPI2X);
  DDRB |=    (1<<5) | (1<<7);
  DDRB &= ~( (1<<2) | (1<<3) | (1<<4) | (1<<6) );
}

uint8_t DPAUCS_spi_tranceive( uint8_t data ){
  if(!initialized)
    spi_init();
  initialized = true;
  SPDR = data;
  while(!( SPSR & (1<<SPIF) ));
  return SPDR;
}

DPAUCS_SHUTDOWN {
  SPCR  = 0;
  SPSR &= ~(1<<SPI2X);
  initialized = false;
}
