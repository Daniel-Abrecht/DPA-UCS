#include <inttypes.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/driver/spi.h>
#include <DPA/UCS/driver/adelay.h>
#include <DPA/UCS/driver/io_pin.h>
#include <DPA/UCS/driver/ethernet.h>
#include <DPA/UCS/driver/config/enc28j60.h>

#define ENC_MEMORY_SIZE 0x2000
#define ENC_MEMORY_RX_END_TX_START (ENC_MEMORY_SIZE-1518-8)
#define MIN_PIN_HIGH_US_T_10 7

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

enum eie_eir_bits {
  EI_RXER, EI_TXER, RESERVED3, EI_TX,
  EI_LINK, EI_DMA, EI_PKT, EI_INT
};

enum econ1_bits {
  ECON1_BSEL0 , ECON1_BSEL1, ECON1_RXEN , ECON1_TXRTS,
  ECON1_CSUMEN, ECON1_DMAST, ECON1_RXRST, ECON1_TXRST
};

enum econ2_bits {
  RESERVED, RESERVED2, RESERVED3, ECON2_VRPS,
  RESERVED, ECON2_PWRSV, ECON2_PKTDEC, ECON2_AUTOINC
};

enum estat_bits {
  ESTAT_CLKRDY, ESTAT_TXABRT, ESTAT_RXBUSY, RESERVED4,
  ESTAT_LATECOL, RESERVED2, ESTAT_BUFFER, ESTAT_INT
};

enum macon1_bits {
  MACON1_MARXEN, MACON1_PASSALL, MACON1_RXPAUS, MACON1_TXPAUS,
  RESERVED, RESERVED2, RESERVED3, RESERVED4
};

enum macon3_bits {
  MACON3_FULLDPX, MACON3_FRMLNEN, MACON3_HFRMEN, MACON3_PHDREN,
  MACON3_TXCRCEN, MACON3_PADCFG0, MACON3_PADCFG1, MACON3_PADCFG2
};

enum macon4_bits {
  RESERVED, RESERVED2, RESERVED3, RESERVED4,
  MACON4_NOBKOFF, MACON4_BPEN, MACON4_DEFER
};

enum miistat_bits {
  MIISTAT_BUSY, MIISTAT_SCAN, MIISTAT_INVALID
};

enum micmd_bits {
  MICMD_MIIRD, MICMD_MIISCAN
};

enum erxfcon_bits {
  ERXFCON_BCEN, ERXFCON_MCEN, ERXFCON_HTEN, ERXFCON_MPEN,
  ERXFCON_PMEN, ERXFCON_CRCEN, ERXFCON_ANDOR, ERXFCON_UCEN
};

enum phy_register {
  PHY_CON1, PHY_STAT1, PHY_ID1, PHY_ID2,
  PHY_CON2=0x10, PHY_STAT2, PHY_IE, PHY_IR, PHY_LCON
};

enum PHY_CON1_bits {
  PHY_CON1_PDPXMD = 8,
  RESERVED, RESERVED2,
  PHY_CON2_PPWRSV,
  RESERVED, RESERVED2,
  PHY_CON2_PLOOPBACK,
  PHY_CON2_PRST
};

enum PHY_CON2_bits {
  PHY_CON2_HDLDIS = 8,
  RESERVED,
  PHY_CON2_JABBER,
  RESERVED, RESERVED2,
  PHY_CON2_TXDIS,
  PHY_CON2_FRCLNK
};

enum PHY_STAT1_bits {
  PHY_STAT1_JBSTAT=1,
  PHY_STAT1_LLSTAT,
  PHY_STAT1_PHDPX=11,
  PHY_STAT1_PFDPX
};

enum PHY_STAT2_bits {
  PHY_STAT2_PLRITY=5,
  PHY_STAT2_DPXSTAT=9,
  PHY_STAT2_LSTAT,
  PHY_STAT2_COLSTAT,
  PHY_STAT2_RXSTAT,
  PHY_STAT2_TXSTAT
};

enum PHY_LCON_leds {
  PHY_LCON_LED_A = 8,
  PHY_LCON_LED_B = 4
};

#ifndef NO_LOGGING
#define S(X) (const flash char[]){X}
// Datasheet page 41
static flash const char* const flash transmit_state_vector_flags[] = {
  S("CRC Error"),
  S("Length Check Error"),
  S("Length Out of Range"),
  S("Done"),
  S("Multicast"),
  S("Broadcast"),
  S("Packet Defer"),
  S("Excessive Defer"),
  S("Excessive Collision"),
  S("Late Collision"),
  S("Giant"),
  S("Underrun"),
  S("Control Frame"),
  S("Pause Control Frame"),
  S("Backpressure Applie"),
  S("Transmit VLAN Tagged Frame")
};
#undef S
#endif

#define PHY_LCON_RESERVED 0x3000

static inline void enc_set_bank(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address
);

static inline void enc_bit_field_set(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  static const flash char error_message[] = {"Attemp to set a bit from a non-ETH enc28j60 register\n"};
  if( ( address >= 0x40 && address <= 0x65 ) || address == CR_MISTAT )
    DPAUCS_fatal(error_message);
  enc_set_bank(iface,address);
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x80 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
}

static inline void enc_bit_field_clear(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  static const flash char error_message[] = {"Attemp to clear a bit from a non-ETH enc28j60 register\n"};
  if( ( address >= 0x40 && address <= 0x65 ) || address == CR_MISTAT )
    DPAUCS_fatal(error_message);
  enc_set_bank(iface,address);
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0xA0 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
}

static void enc_write_control_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address,
  uint8_t data
){
  enc_set_bank(iface,address);
  if( ( address & 0x1F ) == ( CR_ECON1 & 0x1F ) ){
    iface->econ1 = data;
    iface->bank = data & 0x03;
  }
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( ( address & 0x1F ) | 0x40 );
  DPAUCS_spi_tranceive( data );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
}

static inline uint8_t enc_read_control_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address
){
  enc_set_bank(iface,address);
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( address & 0x1F );
  if( ( address >= 0x40 && address <= 0x65 ) || address == CR_MISTAT )
    DPAUCS_spi_tranceive( 0 ); // MAC and MII registers shift out a dummy byte first
  uint8_t ret = DPAUCS_spi_tranceive( 0 );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
  return ret;
}

static inline void enc_write_buffer_memory(
  struct DPAUCS_enc28j60_interface* iface,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( 0x7A );
  while( size-- )
    DPAUCS_spi_tranceive( *(data++) );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
}

static inline void enc_read_buffer_memory(
  struct DPAUCS_enc28j60_interface* iface,
  uint16_t size,
  uint8_t data[size]
){
  if(!size) return;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( 0x3A );
  while( size-- )
    *(data++) = DPAUCS_spi_tranceive( 0 );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
}

static inline uint16_t enc_read_buffer_uint16(
  struct DPAUCS_enc28j60_interface* iface
){
  uint16_t ret;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( 0x3A );
  ret  = (uint16_t)DPAUCS_spi_tranceive( 0 );
  ret |= (uint16_t)DPAUCS_spi_tranceive( 0 ) << 8;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
  return ret;
}

static inline uint32_t enc_read_buffer_uint32(
  struct DPAUCS_enc28j60_interface* iface
){
  uint32_t ret;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( 0x3A );
  ret  = (uint32_t)DPAUCS_spi_tranceive( 0 );
  ret |= (uint32_t)DPAUCS_spi_tranceive( 0 ) <<  8;
  ret |= (uint32_t)DPAUCS_spi_tranceive( 0 ) << 16;
  ret |= (uint32_t)DPAUCS_spi_tranceive( 0 ) << 24;
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
  return ret;
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
  adelay_wait(AD_SEC/10);
  while( enc_read_control_register( iface, CR_MISTAT ) & (1<<MIISTAT_BUSY) );
}

static inline uint16_t enc_read_phy_register(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t reg
){
  while( enc_read_control_register( iface, CR_MISTAT ) & (1<<MIISTAT_BUSY) );
  enc_write_control_register( iface, CR_MIREGADR, reg );
  enc_write_control_register( iface, CR_MICMD, 1<<MICMD_MIIRD );
  while( enc_read_control_register( iface, CR_MISTAT ) & (1<<MIISTAT_BUSY) );
  enc_write_control_register( iface, CR_MICMD, 0 );
  return (uint16_t)enc_read_control_register( iface, CR_MIRDL )
       | (uint16_t)enc_read_control_register( iface, CR_MIRDH )<<8;
}

static inline void enc_reset(
  struct DPAUCS_enc28j60_interface* iface
){
  iface->bank = -1;
  iface->econ1 = (1<<ECON1_TXRST) | (1<<ECON1_RXRST);
  while( enc_read_control_register( iface, CR_ESTAT ) & ESTAT_CLKRDY );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, false , MIN_PIN_HIGH_US_T_10 );
  DPAUCS_spi_tranceive( 0xFF );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
  while( enc_read_control_register( iface, CR_ESTAT ) & ESTAT_CLKRDY );
  enc_write_phy_register( iface, PHY_LCON, (PHY_LED_OFF<<PHY_LCON_LED_A) | (PHY_LED_OFF<<PHY_LCON_LED_B) | PHY_LCON_RESERVED );
}

static inline void enc_set_bank(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t address
){
  if( !( address & 0x80 ) && iface->bank != (address/0x20) ) // Switch bank if necessary
    enc_write_control_register( iface, CR_ECON1, ( iface->econ1 & ~0x03 ) | (address/0x20) );
}

#define R16(R,V) {R##L,(V)&0xFF},{R##H,(V)>>8}
static void enc_init(
  struct DPAUCS_enc28j60_interface* iface
){
  DPAUCS_io_set_pin_mode( iface->config->slave_select_pin, DPAUCS_IO_PIN_MODE_OUTPUT );
  DPAUCS_io_set_pin( iface->config->slave_select_pin, true , MIN_PIN_HIGH_US_T_10 );
  iface->packet_sent_since_last_check = false;
  enc_reset( iface );
  enc_write_phy_register( iface, PHY_LCON,
    ((iface->config->led_A_init&0xF)<<PHY_LCON_LED_A) |
    ((iface->config->led_B_init&0xF)<<PHY_LCON_LED_B) |
    PHY_LCON_RESERVED
  );
  // Enable full dublex or half dublex
  enc_write_phy_register( iface, PHY_CON1, (iface->config->full_dublex<<PHY_CON1_PDPXMD) );
  enc_write_phy_register( iface, PHY_CON2, 1<<PHY_CON2_HDLDIS );
  const uint8_t settings[][2] = {
    // Setup receive buffer, start at 0x0, end at ENC_MEMORY_RX_END_TX_START-1
    R16(CR_ERXST,0), R16(CR_ERXRDPT,0), R16(CR_ERDPT,0), R16(CR_ERXND,ENC_MEMORY_RX_END_TX_START-1),
    // Setup transmit buffer, start where receive buffer ended, give it the rest of all memory
    R16(CR_ETXST,ENC_MEMORY_RX_END_TX_START), R16(CR_ETXND,ENC_MEMORY_SIZE-1),
    // Enable packet receive, transmit, and writing into RX buffer
    { CR_MACON1, (1<<MACON1_MARXEN) | (1<<MACON1_RXPAUS) | (1<<MACON1_TXPAUS) },
    // Append CRC and padding to ethernet frames before transmission, check frame size, enable full or half dublex
    { CR_MACON3, (1<<MACON3_TXCRCEN) | (1<<MACON3_PADCFG0) | (1<<MACON3_PADCFG2) | (1<<MACON3_FRMLNEN) | (iface->config->full_dublex<<MACON3_FULLDPX) },
    // Wait while medium is occupied
    { CR_MACON4, (1<<MACON4_DEFER) },
    // Set Back-to-Back inter packet gap
    { CR_MABBIPG, 0x15 },
    // Set non-Back-to-Back inter packet gap
    R16( CR_MAIPG, 0x12 ),
    // Biggest possible ethernet frame
    R16( CR_MAMXFL, 1518 ),
    // Set mac address
    { CR_MAADR1, iface->super.mac[0] },
    { CR_MAADR2, iface->super.mac[1] },
    { CR_MAADR3, iface->super.mac[2] },
    { CR_MAADR4, iface->super.mac[3] },
    { CR_MAADR5, iface->super.mac[4] },
    { CR_MAADR6, iface->super.mac[5] },
    // Only accept packages with valid crc, for my mac, multicast or broadcast
    { CR_ERXFCON, (1<<ERXFCON_CRCEN) | (1<<ERXFCON_UCEN) | (1<<ERXFCON_MCEN) | (1<<ERXFCON_BCEN) },
    // Automatically increment buffer read/write pointers
    { CR_ECON2, 1<<ECON2_AUTOINC },
    // Change RX and TX from reset state to normal state, enable receiving packages
    { CR_ECON1, ( iface->econ1 & ~( (1<<ECON1_RXRST) | (1<<ECON1_TXRST) ) ) | (1<<ECON1_RXEN) }
  };
  for( unsigned i=0; i<sizeof(settings)/sizeof(*settings); i++ ){
    enc_write_control_register( iface, settings[i][0], settings[i][1] );
    if( settings[i][0] == CR_ERXRDPTL )
      continue; // Skip check for buffered register, it would return the old value
    uint8_t res = enc_read_control_register( iface, settings[i][0] );
    if( res != settings[i][1] )
      DPA_LOG( "Register not set correctly: i: %u reg: %hhx, expected: %hhx, value: %hhx\n", i, settings[i][0], settings[i][1], res );
  }
  enc_write_phy_register( iface, PHY_LCON,
    ((iface->config->led_A&0xF)<<PHY_LCON_LED_A) |
    ((iface->config->led_B&0xF)<<PHY_LCON_LED_B) |
    PHY_LCON_RESERVED
  );
}
#undef R16

static void eth_init( void ){
  driver.interface_list = (struct DPAUCS_interface_list*)DPAUCS_enc28j60_interface_list;
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    enc_init( it->entry );
  }
}

/*static inline void dump_last_packet_in_send_buffer(
  struct DPAUCS_enc28j60_interface* iface
){
  static const flash char hex[] = {"0123456789ABCDEF"};
  uint16_t rptr_tmp = (uint16_t)enc_read_control_register( iface, CR_ERDPTL ) | (uint16_t)enc_read_control_register( iface, CR_ERDPTH )<<8;
  uint16_t wpkg_end = (uint16_t)enc_read_control_register( iface, CR_ETXNDL ) | (uint16_t)enc_read_control_register( iface, CR_ETXNDH )<<8;
  enc_write_control_register( iface, CR_ERDPTL, (ENC_MEMORY_RX_END_TX_START+1) & 0xFF );
  enc_write_control_register( iface, CR_ERDPTH, (ENC_MEMORY_RX_END_TX_START+1) >> 8 );
  uint16_t length = wpkg_end - ENC_MEMORY_RX_END_TX_START;
  DPA_LOG("Dumping last packet in send buffer, size: %"PRIu16" bytes:\n",length);
  for( uint16_t i=0; i<length; i++ ){
    uint8_t val;
    enc_read_buffer_memory( iface, 1, &val );
    putchar(hex[val>>4]);
    putchar(hex[val&0xF]);
    if( (i+1) % 16 ){
      putchar(' ');
    }else{
      putchar('\n');
    }
  }
  putchar('\n');
  enc_write_control_register( iface, CR_ERDPTL, rptr_tmp & 0xFF );
  enc_write_control_register( iface, CR_ERDPTH, rptr_tmp >> 8 );
}*/

static bool eth_tx_ready(
  struct DPAUCS_enc28j60_interface* iface
){
  if( !iface->packet_sent_since_last_check )
    return true;
  bool res = !( enc_read_control_register( iface, CR_ECON1 ) & (1<<ECON1_TXRTS) );
  if( res )
    iface->packet_sent_since_last_check = false;
  return res;
}

static inline void get_transmit_state_vector(
  struct DPAUCS_enc28j60_interface* iface,
  uint8_t sv[7]
){
  uint16_t readptr_tmp = enc_read_control_register( iface, CR_ERDPTL )
                        | enc_read_control_register( iface, CR_ERDPTH );
  uint16_t svptr = enc_read_control_register( iface, CR_ERXNDL )
                  | enc_read_control_register( iface, CR_ERXNDH );
  enc_write_control_register( iface, CR_ERDPTL, svptr & 0xFF );
  enc_write_control_register( iface, CR_ERDPTH, svptr >> 8 );
  enc_read_buffer_memory( iface, 7, sv );
  enc_write_control_register( iface, CR_ERDPTL, readptr_tmp & 0xFF );
  enc_write_control_register( iface, CR_ERDPTH, readptr_tmp >> 8 );
}

static inline void print_transmit_status_vector(
  const uint8_t sv[7]
){
  (void)sv;
  DPA_LOG( "Transmit status vector:\n" );
  for( unsigned i=20,j=0; i<52; i++,j++ ){
    if( i == 32 )
      i = 48;
    if( sv[i>>3] & (1<<(i&0x7)) )
      DPA_LOG( " - %"PRIsFLASH"\n", transmit_state_vector_flags[j] );
  }
  DPA_LOG( " - Byte Count %u\n", sv[0] | sv[1]<<8 );
  DPA_LOG( " - Total Bytes Transmitted on Wire %u\n", sv[4] | sv[5]<<8 );
  DPA_LOG( " - Collision Count %u\n", sv[2] & 0xF );
}

static void check_print_error_info(
  struct DPAUCS_enc28j60_interface* iface
){
  uint8_t estat = enc_read_control_register( iface, CR_ESTAT );
  if( estat & (1<<ESTAT_TXABRT) ){
    DPA_LOG( "Sending last package failed\n" );
    if( estat & (1<<ESTAT_LATECOL) )
      DPA_LOG( " - Late collision error\n" );
    uint8_t sv[7];
    get_transmit_state_vector( iface, sv );
    print_transmit_status_vector( sv );
  }
}

static void check_link_state(
  struct DPAUCS_enc28j60_interface* iface
){
  bool new_link_state = enc_read_phy_register( iface, PHY_STAT1 ) & (1<<PHY_STAT1_LLSTAT);
  if( iface->super.link_up != new_link_state ){
    if( new_link_state ){
      DPA_LOG("ENC28J60 link up\n");
      DPAUCS_interface_event(&iface->super,DPAUCS_IFACE_EVENT_LINK_UP,0);
    }else{
      DPA_LOG("ENC28J60 link down\n");
      DPAUCS_interface_event(&iface->super,DPAUCS_IFACE_EVENT_LINK_DOWN,0);
    }
  }
}

static void eth_send( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t length ){
  struct DPAUCS_enc28j60_interface* iface = (struct DPAUCS_enc28j60_interface*)interface;
  if( length > 1518 ){
    DPA_LOG( "Ethernet packet too big. %"PRIu16" > 1518\n", length );
    return;
  }
  DPA_LOG("eth_send %"PRIu16" bytes\n",length);
  adelay_t adelay;
  adelay_start(&adelay);
  int i = 0;
  while( !eth_tx_ready(iface) ){
    if( adelay_done(&adelay,AD_SEC) ){
      if( i++ < 3 ){
        DPA_LOG("ENC28J60 stalled, last ethernet packet wasn't sent within 3 seconds\n");
        check_print_error_info( iface );
        DPA_LOG("Trying to reinitialise ENC28J60, attemp %d...\n",i);
        enc_init( iface );
        adelay_start(&adelay);
      }else{
        static const flash char message[] = {"ENC28J60 didn't react, even after 3 reset attemps!\n"};
        DPAUCS_fatal(message);
      }
    }
  }
  if( enc_read_control_register( iface, CR_EIR ) & (1<<EI_TXER) )
    check_print_error_info( iface );
  enc_bit_field_clear( iface, CR_EIR, (1<<EI_TXER) );
  enc_bit_field_clear( iface, CR_ESTAT, (1<<ESTAT_TXABRT) | (1<<ESTAT_LATECOL) );
  // Set write pointer to buffer start
  enc_write_control_register( iface, CR_EWRPTL, ENC_MEMORY_RX_END_TX_START & 0xFF );
  enc_write_control_register( iface, CR_EWRPTH, ENC_MEMORY_RX_END_TX_START >> 8 );
  // Set write buffer data end pointer
  uint16_t end = ENC_MEMORY_RX_END_TX_START + length;
  enc_write_control_register( iface, CR_ETXNDL, end & 0xFF );
  enc_write_control_register( iface, CR_ETXNDH, end >> 8 );
  // ENC28J60 packet control byte, 0=use defaults from CR_MACON3
  enc_write_buffer_memory( iface, 1, (uint8_t[]){0} );
  // Write ethernet frame into buffer
  enc_write_buffer_memory( iface, length, packet );
  // Send packet
//  dump_last_packet_in_send_buffer(iface);
  enc_bit_field_set( iface, CR_ECON1, 1<<ECON1_TXRTS );
  iface->packet_sent_since_last_check = true;
}

static uint16_t eth_receive( const DPAUCS_interface_t* interface, uint8_t* packet, uint16_t maxlen ){
  struct DPAUCS_enc28j60_interface* iface = (struct DPAUCS_enc28j60_interface*)interface;
  // Check if a packet was received
  uint8_t count = enc_read_control_register( iface, CR_EPKTCNT );
  if( !count ) return 0; // No new packages
  // Receive buffer contains some additional informations before the package datas
  uint16_t next_packet_pointer = enc_read_buffer_uint16( iface );
  (void)enc_read_buffer_uint16( iface );
  (void)enc_read_buffer_uint16( iface );
  uint16_t receive_buffer_pointer =   enc_read_control_register( iface, CR_ERDPTL )
                                  | ( enc_read_control_register( iface, CR_ERDPTH ) << 8 );
  uint16_t length;
  // Ringbuffer, detect pointer overflow
  if( receive_buffer_pointer < next_packet_pointer ){
    length = next_packet_pointer - receive_buffer_pointer;
  }else{
    length = ( ENC_MEMORY_RX_END_TX_START - receive_buffer_pointer ) + next_packet_pointer;
  }
  if( length > maxlen ){
    DPA_LOG("Packet in receive buffer too big: %"PRIu16" > %"PRIu16"\n", length, maxlen );
    enc_write_control_register( iface, CR_ERDPTL, next_packet_pointer & 0xFF );
    enc_write_control_register( iface, CR_ERDPTH, next_packet_pointer >> 8 );
  }else{
    enc_read_buffer_memory( iface, length, packet );
  }
  // Free packet memory in read buffer
  enc_write_control_register( iface, CR_ERXRDPTL, next_packet_pointer & 0xFF );
  enc_write_control_register( iface, CR_ERXRDPTH, next_packet_pointer >> 8 );
  // Decrement packet count
  enc_bit_field_set( iface, CR_ECON2, 1<<ECON2_PKTDEC );
  DPA_LOG("eth_receive %"PRIu16"\n",length);
  return length;
}

static void eth_shutdown( void ){
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    enc_reset( it->entry );
  }
}

DPAUCS_TASK {
  static uint8_t check = 0;
  if( check++ )
    return;
  for( struct DPAUCS_enc28j60_interface_list* it = DPAUCS_enc28j60_interface_list; it; it = it->next ){
    check_link_state( it->entry );
    if( !it->entry->super.link_up )
      continue;
    if( eth_tx_ready( it->entry ) ){
      if( enc_read_control_register( it->entry, CR_EIR ) & (1<<EI_TXER) ){
        check_print_error_info( it->entry );
        enc_bit_field_clear( it->entry, CR_EIR, (1<<EI_TXER) );
      }
    }
  }
}
