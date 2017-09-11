#include <DPA/UCS/driver/config/enc28j60.h>
#include <DPA/UCS/driver/config/spi.h>

ENC28J60(
  (00,22,19,01,E2,87),
  (
    .slave_select_pin = 12,
    .interrupt_pin    = 10
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
