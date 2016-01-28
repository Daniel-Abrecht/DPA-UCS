#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/protocol/address.h>

#define PACKET_SIZE 1518
#define PACKET_SIZE_WITH_CHECKSUM PACKET_SIZE + 4
#define PACKET_MAX_PAYLOAD 1500
#define IEEE_802_1Q_TPID_CONST 0x8100

typedef PACKED1 struct PACKED2 DPAUCS_LLC {
  uint8_t dsap; // ???
  uint8_t ssap; // ???
  PACKED1 union PACKED2 {
    PACKED1 struct PACKED2 {
      uint8_t control; // ???
      uint8_t payload[PACKET_MAX_PAYLOAD];
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
        uint8_t payload[PACKET_MAX_PAYLOAD];
      };
    } extended_control;
  };
} DPAUCS_LLC_t;

typedef struct DPAUCS_packet { // Ethernet frame
  uint16_t size;
  union {
    uint8_t raw[PACKET_SIZE_WITH_CHECKSUM];
    PACKED1 struct PACKED2 {
      DPAUCS_mac_t dest; // MAC destination
      DPAUCS_mac_t src; // MAC source
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
            DPAUCS_LLC_t llc; // length was specified before
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
            DPAUCS_LLC_t llc; // length was specified before
          };
        };
      };
    };
  } data;
} DPAUCS_packet_t;

typedef struct DPAUCS_packet_info {
  DPAUCS_mac_t destination_mac;
  DPAUCS_mac_t source_mac;
  const struct DPAUCS_interface* interface;
  struct DPAUCS_LLC* llc;
  void* payload;
  uint16_t vid;
  uint16_t type;
  uint16_t size;
  bool is_vlan;
  bool invalid;
} DPAUCS_packet_info_t;

#endif
