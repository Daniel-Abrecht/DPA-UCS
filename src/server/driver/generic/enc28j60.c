#include <DPA/UCS/driver/spi.h>
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

enum bank_0 {
  R_ERDPTL,
  R_ERDPTH,
  R_EWRPTL,
  R_EWRPTH,
  R_ETXSTL,
  R_ETXSTH,
  R_ETXNDL,
  R_ETXNDH
};

static inline void enc28j60_reset(
  const flash struct DPAUCS_enc28j60_config* config
){
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  DPAUCS_spi_tranceive( 0xFF );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static inline void enc28j60_bit_field_set(
  const flash struct DPAUCS_enc28j60_config* config,
  uint8_t address,
  uint8_t data
){
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x80 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static inline void enc28j60_bit_field_clear(
  const flash struct DPAUCS_enc28j60_config* config,
  uint8_t address,
  uint8_t data
){
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0xA0 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static inline void enc28j60_write_control_register(
  const flash struct DPAUCS_enc28j60_config* config,
  uint8_t address,
  uint8_t data
){
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x40 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static inline uint8_t enc28j60_read_control_register(
  const flash struct DPAUCS_enc28j60_config* config,
  uint8_t address
){
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  uint8_t ret = DPAUCS_spi_tranceive( address & 0x1F );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
  return ret;
}

static inline void enc28j60_write_buffer_memory(
  const flash struct DPAUCS_enc28j60_config* config,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  DPAUCS_spi_tranceive( 0x7A );
  while( size-- )
    DPAUCS_spi_tranceive( *(data++) );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static inline void enc28j60_read_buffer_memory(
  const flash struct DPAUCS_enc28j60_config* config,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( config->slave_select_pin, false );
  while( size-- )
    *(data++) = DPAUCS_spi_tranceive( 0x3A );
  DPAUCS_io_set_pin( config->slave_select_pin, true );
}

static void eth_init( void ){
  driver.interface_list = (struct DPAUCS_interface_list*)DPAUCS_enc28j60_interface_list;
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    DPAUCS_io_set_pin_mode( it->entry->config->interrupt_pin, DPAUCS_IO_PIN_MODE_INPUT );
    DPAUCS_io_set_pin_mode( it->entry->config->slave_select_pin, DPAUCS_IO_PIN_MODE_OUTPUT );
    DPAUCS_io_set_pin( it->entry->config->slave_select_pin, true );
    enc28j60_reset( it->entry->config );
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

static void eth_shutdown( void ){
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    enc28j60_reset( it->entry->config );
  }
}
