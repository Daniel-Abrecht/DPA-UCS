#include <eth.h>
#include <drivers/enc28j60.h>

void DPAUCS_ethInit( uint8_t* macaddr ){
  enc28j60Init( macaddr );
  enc28j60PhyWrite( PHLCON, 0x476 );
  enc28j60clkout( 2 );
}

void DPAUCS_ethSend( uint8_t* packet, uint16_t len ){
  enc28j60PacketSend( len, packet );
}

uint16_t DPAUCS_ethReceive( uint8_t* packet, uint16_t maxlen ){
  return enc28j60PacketReceive( maxlen, packet );
}

