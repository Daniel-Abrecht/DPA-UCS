#include <DPA/UCS/driver/io_pin.h>
#include <DPA/UCS/driver/ethernet.h>
#include <DPA/UCS/driver/config/enc28j60.h>


static void eth_init( void );
static void eth_send( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t len );
static uint16_t eth_receive( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t maxlen );
static void eth_shutdown( void );


static DPAUCS_ethernet_driver_t driver = {
  .name       = "enc28j60",
  .init       = &eth_init,
  .send       = &eth_send,
  .receive    = &eth_receive,
  .shutdown   = &eth_shutdown,
};
DPA_LOOSE_LIST_ADD( DPAUCS_ethernet_driver_list, &driver )


static void eth_init( void ){
  driver.interface_list = (struct DPAUCS_interface_list*)DPAUCS_enc28j60_interface_list;
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    DPAUCS_io_set_pin_mode( it->entry->config->interrupt_pin, DPAUCS_IO_PIN_MODE_INPUT );
    DPAUCS_io_set_pin_mode( it->entry->config->slave_select_pin, DPAUCS_IO_PIN_MODE_OUTPUT );
    DPAUCS_io_set_pin( it->entry->config->slave_select_pin, true );
  }
}

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
