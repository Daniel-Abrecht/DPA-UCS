#include <DPA/UCS/driver/eth/driver.h>

static DPAUCS_interface_t interfaces;

static void eth_init( void ){}

static void eth_send( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t len ){
  (void)interface;
  (void)packet;
  (void)len;
}

static uint16_t eth_receive( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t maxlen ){
  (void)interface;
  (void)packet;
  (void)maxlen;
  return 0;
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

DPAUCS_EXPORT_ETHERNET_DRIVER( dummy, &driver );