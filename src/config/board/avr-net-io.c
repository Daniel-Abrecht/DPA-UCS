#include <DPA/UCS/driver/config/enc28j60.h>
#include <DPA/UCS/driver/config/spi.h>

ENC28J60(
  (00,22,19,01,E2,87),
  (
    .slave_select_pin = 12,
    .interrupt_pin = 10,
    .full_dublex = true,
    .led_A = PHY_LED_DISPLAY_TRANSMIT_AND_RECEIVE_ACTIVITY,
    .led_B = PHY_LED_DISPLAY_LINK_STATUS,
    .led_A_init = PHY_LED_ON,
    .led_B_init = PHY_LED_ON
  )
)

SPI(
  .master = true,
  .mode = 0,
  .mosi = 13,
  .miso = 14,
  .sck  = 15,
  .clock_rate = 2
)
