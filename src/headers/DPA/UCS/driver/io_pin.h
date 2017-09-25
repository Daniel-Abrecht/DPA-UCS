#ifndef DPAUCS_IO_PIN_DRIVER_H
#define DPAUCS_IO_PIN_DRIVER_H

#include <stdbool.h>

typedef enum DPAUCS_io_pin_mode {
  DPAUCS_IO_PIN_MODE_INPUT,
  DPAUCS_IO_PIN_MODE_OUTPUT
} DPAUCS_io_pin_mode_t;

void DPAUCS_io_set_pin_mode( unsigned pin, DPAUCS_io_pin_mode_t mode );
void DPAUCS_io_set_pin( unsigned pin, bool state, uint16_t delay_us_t_10 );
bool DPAUCS_io_get_pin( unsigned pin );

#endif
