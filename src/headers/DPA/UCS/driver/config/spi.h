#ifndef DPAUCS_CONFIG_SPI_H
#define DPAUCS_CONFIG_SPI_H

struct spi_config {
  bool master;
  uint8_t mode:2;
  unsigned mosi, miso, sck;
  unsigned clock_rate;
};

extern const flash struct spi_config spi_config;

#define SPI(...) const flash struct spi_config spi_config = {__VA_ARGS__};

#endif
