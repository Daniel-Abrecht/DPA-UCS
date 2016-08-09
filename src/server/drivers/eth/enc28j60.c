#include <DPA/UCS/drivers/enc28j60.h>
#include <DPA/UCS/driver/eth/driver.h>

#ifndef ENC28J60_MAC
#define ENC28J60_MAC (uint8_t[]){ 0x5C, 0xF9, 0xDD, 0x55, 0x96, 0xC2 }
#endif

static DPAUCS_interface_t interfaces = {
  .mac = ENC28J60_MAC
};

static void eth_init( void ){
  enc28j60Init( interface.mac );
  enc28j60PhyWrite( PHLCON, 0x476 );
  enc28j60clkout( 2 );
}

static void eth_send( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t len ){
  (void)interface;
  enc28j60PacketSend( len, packet );
}

static uint16_t eth_receive( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t maxlen ){
  (void)interface;
  return enc28j60PacketReceive( maxlen, packet );
}

static void eth_shutdown( void ){}

static DPAUCS_ethernet_driver_t driver = {
  .init       = &eth_init,
  .send       = &eth_send,
  .receive    = &eth_receive,
  .shutdown   = &eth_shutdown,
  .interfaces = &interfaces,
  .interface_count = 1
};

DPAUCS_EXPORT_ETHERNET_DRIVER( enc28j60, &driver );
