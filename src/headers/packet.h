#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <packed.h>

#define PACKET_SIZE 1522
#define PACKET_MAX_PAYLOAD 1500
#define IEEE_802_1Q_TPID_CONST 0x8100

typedef PACKED1 struct PACKED2 {
  uint8_t dsap; // ???
  uint8_t ssap; // ???
  PACKED1 union PACKED2 {
    PACKED1 struct PACKED2 {
      uint8_t control; // ???
      uint8_t payload[1500];
    } control;
    PACKED1 struct PACKED2 {
      uint16_t control;
      PACKED1 union PACKED2 {
        PACKED1 struct PACKED2 {
          PACKED1 union PACKED2 {
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
    PACKED1 struct PACKED2 {
      uint8_t dest[6]; // MAC destination
      uint8_t src[6]; // MAC source
      PACKED1 union PACKED2 {
        PACKED1 struct PACKED2 { // IEEE_802_1Q
          uint16_t tpid; // Tag protocol identifier: 0x8100
          // Priority code point 
          // Drop eligible indicator 
          // VLAN identifier
          uint16_t pcp_dei_vid; // 3 1 12
          PACKED1 union PACKED2 {
            uint16_t type; // greater or equal to 1536
            uint16_t length; // smaller or equal to 1500
          };
          PACKED1 union PACKED2 {
            uint8_t payload[PACKET_MAX_PAYLOAD]; // type was specified before
            DPAUCS_LLC llc; // length was specified before
          };
        } vlan; // IEEE_802_1Q
        PACKED1 struct PACKED2 {
          PACKED1 union PACKED2 {
            uint16_t tpid; // Tag protocol identifier: 0x8100
            uint16_t type; // greater or equal to 1536
            uint16_t length; // smaller or equal to 1500
          };
          PACKED1 union PACKED2 {
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
