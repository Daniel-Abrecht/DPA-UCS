#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>

#define PACKET_SIZE 1522
#define PACKET_MAX_PAYLOAD 1500
#define IEEE_802_1Q_TPID_CONST 0x8100

typedef struct __attribute__ ((__packed__)) {
  uint8_t dsap; // ???
  uint8_t ssap; // ???
  union __attribute__ ((__packed__)) {
    struct __attribute__ ((__packed__)) {
      uint8_t control; // ???
      uint8_t payload[1500];
    } control;
    struct __attribute__ ((__packed__)) {
      uint16_t control;
      union __attribute__ ((__packed__)) {
        struct __attribute__ ((__packed__)) {
          union __attribute__ ((__packed__)) {
            unsigned oui : 3;
            uint8_t oui_bytes[3];
          };
          uint16_t type; // Should contain ethernet type filed value
        } snap; // only present if DSAP equals 0xAA
        uint8_t payload[1500];
      };
    } extended_control;
  };
} DPAUCS_LLC;

typedef struct { // Ethernet frame
  uint16_t size;
  union {
    uint8_t raw[PACKET_SIZE];
    struct __attribute__ ((__packed__)) {
      uint8_t dest[6]; // MAC destination
      uint8_t src[6]; // MAC source
      union __attribute__ ((__packed__)) {
        struct __attribute__ ((__packed__)) { // IEEE_802_1Q
          uint16_t tpid; // Tag protocol identifier: 0x8100
          unsigned pcp:3; // Priority code point 
          unsigned dei:1; // Drop eligible indicator 
          unsigned vid:12; // VLAN identifier
          union __attribute__ ((__packed__)) {
            uint16_t type; // greater or equal to 1536
            uint16_t length; // smaller or equal to 1500
          };
          union __attribute__ ((__packed__)) {
            uint8_t payload[PACKET_MAX_PAYLOAD]; // type was specified before
            DPAUCS_LLC llc; // length was specified before
          };
        } vlan; // IEEE_802_1Q
        struct __attribute__ ((__packed__)) {
          union __attribute__ ((__packed__)) {
            uint16_t tpid; // Tag protocol identifier: 0x8100
            uint16_t type; // greater or equal to 1536
            uint16_t length; // smaller or equal to 1500
          };
          union __attribute__ ((__packed__)) {
            uint8_t payload[PACKET_MAX_PAYLOAD]; // type was specified before
            DPAUCS_LLC llc; // length was specified before
          };
        };
      };
    };
  } data;
} DPAUCS_packet_t;

typedef struct {
  uint8_t destination_mac[6];
  uint8_t source_mac[6];
  bool is_vlan;
  uint16_t vid;
  uint16_t type;
  void* payload;
  DPAUCS_LLC* llc;
  bool invalid;
} DPAUCS_packet_info;

#endif
