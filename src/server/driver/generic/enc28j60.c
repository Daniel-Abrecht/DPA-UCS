#include <DPA/utils/logger.h>
#include <DPA/UCS/driver/spi.h>
#include <DPA/UCS/driver/io_pin.h>
#include <DPA/UCS/driver/ethernet.h>
#include <DPA/UCS/driver/config/enc28j60.h>

#define RESERVED DPA_CONCAT( R_RESERVED_, __LINE__ )
#define RESERVED2 DPA_CONCAT( R_RESERVED2_, __LINE__ )
#define RESERVED3 DPA_CONCAT( R_RESERVED3_, __LINE__ )
#define RESERVED4 DPA_CONCAT( R_RESERVED4_, __LINE__ )


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

enum control_register {
// Bank 0
  CR_ERDPTL=0x0 , CR_ERDPTH  , CR_EWRPTL  , CR_EWRPTH  ,
  CR_ETXSTL     , CR_ETXSTH  , CR_ETXNDL  , CR_ETXNDH  ,
  CR_ERXSTL     , CR_ERXSTH  , CR_ERXNDL  , CR_ERXNDH  ,
  CR_ERXRDPTL   , CR_ERXRDPTH, CR_ERXWRPTL, CR_ERXWRPTH,
  CR_EDMASTL    , CR_EDMASTH , CR_EDMANDL , CR_EDMANDH ,
  CR_EDMADSTL   , CR_EDMADSTH, CR_EDMACSL , CR_EDMACSH ,

// Bank 1
  CR_EHT0=0x20  , CR_EHT1    , CR_EHT2    , CR_EHT3    ,
  CR_EHT4       , CR_EHT5    , CR_EHT6    , CR_EHT7    ,
  CR_EPMM0      , CR_EPM1    , CR_EPM2    , CR_EPM3    ,
  CR_EPMM4      , CR_EPM5    , CR_EPM6    , CR_EPM7    ,
  CR_EPMCSL     , CR_EPMCSH  , RESERVED3  , RESERVED4  ,
  CR_EPMOL      , CR_EPMOH   , RESERVED3  , RESERVED4  ,
  CR_ERXFCON    , CR_EPKTCNT ,

// Bank 2
  CR_MACON1=0x40, RESERVED2  , CR_MACON3  , CR_MACON4  ,
  CR_MABBIPG    , RESERVED2  , CR_MAIPGL  , CR_MAIPGH  ,
  CR_MACLCON1   , CR_MACLCON2, CR_MAMXFLL , CR_MAMXFLH ,
  RESERVED      , RESERVED2  , RESERVED3  , RESERVED4  ,
  RESERVED      , RESERVED2  , CR_MICMD   , RESERVED4  ,
  CR_MIREGADR   , RESERVED2  , CR_MIWRL   , CR_MIWRH   ,
  CR_MIRDL      , CR_MIRDH   ,

// Bank 3
  CR_MAADR5=0x60, CR_MAADR6  , CR_MAADR3  , CR_MAADR4  ,
  CR_MAADR1     , CR_MAADR2  , CR_EBSTSD  , CR_EBSTCON ,
  CR_EBSTCSL    , CR_EBSTCSH , CR_MISTAT  , RESERVED4  ,
  RESERVED      , RESERVED2  , RESERVED3  , RESERVED4  ,
  RESERVED      , RESERVED2  , CR_EREVID  , RESERVED4  ,
  RESERVED      , CR_ECOCON  , RESERVED3  , CR_EFLOCON ,
  CR_EPAUSL     , CR_EPAUSH  ,

// Any Bank
  RESERVED=0x9A ,  CR_EIE    , CR_EIR     , CR_ESTAT   ,
  CR_ECON2      ,  CR_ECON1
};

enum econ1_bits {
  ECON1_BSEL0 , ECON1_BSEL1, ECON1_RXEN , ECON1_TXRTS,
  ECON1_CSUMEN, ECON1_DMAST, ECON1_RXRST, ECON1_TXRST
};

enum miistat_bits {
  MIISTAT_BUSY, MIISTAT_SCAN, MIISTAT_INVALID
};

enum phy_register {
  PHY_CON1, PHY_STAT1, PHY_ID1, PHY_ID2,
  PHY_CON2=0x10, PHY_STAT2, PHY_IE, PHY_IR, PHY_LCON
};

enum PHY_LCON_led_mode {
  RESERVED,
  PHY_LED_DISPLAY_TRANSMIT_ACTIVITY,
  PHY_LED_DISPLAY_RECEIVE_ACTIVITY,
  PHY_LED_DISPLAY_COLLISION_ACTIVITY,
  PHY_LED_DISPLAY_LINK_STATUS,
  PHY_LED_DISPLAY_DUBLEX_STATUS,
  RESERVED,
  PHY_LED_DISPLAY_TRANSMIT_AND_RECEIVE_ACTIVITY,
  PHY_LED_ON,
  PHY_LED_OFF,
  PHY_LED_BLINK_FAST,
  PHY_LED_BLINK_SLOW,
  PHY_LED_DISPLAY_LINK_STATUS_AND_RECEIVE_ACTIVITY,
  PHY_LED_DISPLAY_LINK_STATUS_AND_TRANSMIT_AND_RECEIVE_ACTIVITY,
  PHY_LED_DISPLAY_DUBLEX_STATUS_AND_COLLISION_ACTIVITY,
  RESERVED
};

enum PHY_LCON_leds {
  PHY_LCON_LED_ORANGE = 8,
  PHY_LCON_LED_GREEN = 4
};

#define PHY_LCON_RESERVED 0x3000

static inline void enc_reset(
  struct DPAUCS_enc28j60_interface* iface
){
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  DPAUCS_spi_tranceive( 0xFF );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static inline void enc_bit_field_set(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x80 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static inline void enc_bit_field_clear(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0xA0 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static void enc_write_control_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  if( !( address & 0x80 ) && iface->bank != address/0x20 )
    enc_write_control_register( iface, CR_ECON1, ( ~iface->econ1 & 0x03 ) | (address/0x20) );
  if( ( address & 0x1F ) == ( CR_ECON1 & 0x1F ) ){
    iface->econ1 = data;
    iface->bank = data & 0x03;
  }
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x40 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static inline uint8_t enc_read_control_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address
){
  if( !( address & 0x80 ) && iface->bank != address/0x20 )
    enc_write_control_register( iface, CR_ECON1, ( ~iface->econ1 & 0x03 ) | (address/0x20) );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  uint8_t ret = DPAUCS_spi_tranceive( address & 0x1F );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
  return ret;
}

static inline void enc_write_buffer_memory(
  struct DPAUCS_enc28j60_interface* iface,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  DPAUCS_spi_tranceive( 0x7A );
  while( size-- )
    DPAUCS_spi_tranceive( *(data++) );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static inline void enc_read_buffer_memory(
  struct DPAUCS_enc28j60_interface* iface,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false );
  while( size-- )
    *(data++) = DPAUCS_spi_tranceive( 0x3A );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true );
}

static void enc_write_phy_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t reg,
  uint16_t data
){
  while( enc_read_control_register( iface, CR_MISTAT ) & (1<<MIISTAT_BUSY) );
  enc_write_control_register( iface, CR_MIREGADR, reg );
  enc_write_control_register( iface, CR_MIWRL, data );
  enc_write_control_register( iface, CR_MIWRH, data >> 8 );
}

static void eth_init( void ){
  driver.interface_list = (struct DPAUCS_interface_list*)DPAUCS_enc28j60_interface_list;
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    DPAUCS_io_set_pin_mode( it->entry->config->interrupt_pin, DPAUCS_IO_PIN_MODE_INPUT );
    DPAUCS_io_set_pin_mode( it->entry->config->slave_select_pin, DPAUCS_IO_PIN_MODE_OUTPUT );
    DPAUCS_io_set_pin( it->entry->config->slave_select_pin, true );
    it->entry->bank = -1;
    it->entry->econ1 = (1<<ECON1_TXRST) | (1<<ECON1_RXRST);
    enc_reset( it->entry );
    enc_write_phy_register( it->entry, PHY_LCON, (PHY_LED_ON<<PHY_LCON_LED_ORANGE)  | (PHY_LED_ON<<PHY_LCON_LED_GREEN)  | PHY_LCON_RESERVED );
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
    enc_reset( it->entry );
  }
}
