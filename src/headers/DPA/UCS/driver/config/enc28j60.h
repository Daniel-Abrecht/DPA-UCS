#ifndef DPAUCS_CONFIG_ENC28J60_H
#define DPAUCS_CONFIG_ENC28J60_H

#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/address.h>

// See src/config/board/avr-net-io.c for an example config

#define ENC28J60( MAC, PARAMS ) \
  static const flash struct DPAUCS_enc28j60_config DPA_CONCAT( enc28j60_config_, __LINE__ ) = { \
    DPA_UNPACK PARAMS \
  }; \
  static struct DPAUCS_enc28j60_interface DPA_CONCAT( enc28j60_interface_, __LINE__ ) = { \
    .super = { .mac = DPAUCS_MAC MAC }, \
    .config = &DPA_CONCAT( enc28j60_config_, __LINE__ ) \
  }; \
  DPA_LOOSE_LIST_ADD( DPAUCS_enc28j60_interface_list, &DPA_CONCAT( enc28j60_interface_, __LINE__ ) )

struct enc28j60_config;
struct enc28j60_interface;

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

struct DPAUCS_enc28j60_config {
  unsigned slave_select_pin;
  unsigned interrupt_pin;
  bool full_dublex;
  enum PHY_LCON_led_mode led_A_init;
  enum PHY_LCON_led_mode led_B_init;
  enum PHY_LCON_led_mode led_A;
  enum PHY_LCON_led_mode led_B;
};

struct DPAUCS_enc28j60_interface {
  struct DPAUCS_interface super;
  const flash struct DPAUCS_enc28j60_config* config;
  uint8_t econ1, bank;
};

DPA_LOOSE_LIST_DECLARE( struct DPAUCS_enc28j60_interface*, DPAUCS_enc28j60_interface_list )

#endif
