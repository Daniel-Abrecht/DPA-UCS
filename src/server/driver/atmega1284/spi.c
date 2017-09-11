#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/driver/spi.h>
#include <DPA/UCS/driver/io_pin.h>
#include <DPA/UCS/driver/config/spi.h>

static bool initialized = false;

void spi_init(){
  SPCR = 0;
  uint8_t spcr_tmp = (1<<SPE);
  if( spi_config.master )
    spcr_tmp |= (1<<MSTR);
  if( spi_config.mode & 0x1 )
    spcr_tmp |= (1<<CPHA);
  if( spi_config.mode & 0x2 )
    spcr_tmp |= (1<<CPOL);
  switch( spi_config.clock_rate ){
    case   2: SPSR |= (1<<SPI2X); break;
    case   4: break;
    case   8: SPSR |= (1<<SPI2X); spcr_tmp |= (1<<SPR0); break;
    case  16: spcr_tmp |= (1<<SPR0); break;
    case  32: SPSR |= (1<<SPI2X); spcr_tmp |= (1<<SPR1); break;
    case  64: spcr_tmp |= (1<<SPR1); break;
    case 128: spcr_tmp |= (1<<SPR0) | (1<<SPR1); break;
    default: {
      static const flash char message[] = {"Invalid clock rate specified\n"};
      DPAUCS_fatal(message);
    }
  }
  DPAUCS_io_set_pin_mode( spi_config.mosi, DPAUCS_IO_PIN_MODE_OUTPUT );
  DPAUCS_io_set_pin_mode( spi_config.sck , DPAUCS_IO_PIN_MODE_OUTPUT );
  DPAUCS_io_set_pin( spi_config.mosi, false );
  DPAUCS_io_set_pin( spi_config.sck , false );
  DPAUCS_io_set_pin_mode( spi_config.miso, DPAUCS_IO_PIN_MODE_INPUT );
  SPCR = spcr_tmp;
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
