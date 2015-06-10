#ifndef IPv6_H
#define IPv6_H

typedef PACKED1 struct PACKED2 {
  uint8_t version_tafficClass1;
  uint8_t tafficClass2_flowLabel1;
  uint16_t flowLabel2;
  uint16_t payloadLength;
  uint8_t nextHeader;
  uint8_t hopLimit;
  uint8_t sourceAddress[128];
  uint8_t destinationAddress[128];
} DPAUCS_ipv6_t;

#endif