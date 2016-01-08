#if !defined( IPv6_H ) && defined( USE_IPv6 )
#define IPv6_H

#include <stdint.h>
#include <DPA/UCS/helper_macros.h>

typedef PACKED1 struct PACKED2 DPAUCS_IPv6 {
  uint8_t version_tafficClass1;
  uint8_t tafficClass2_flowLabel1;
  uint16_t flowLabel2;
  uint16_t payloadLength;
  uint8_t nextHeader;
  uint8_t hopLimit;
  uint8_t sourceAddress[128];
  uint8_t destinationAddress[128];
} DPAUCS_IPv6_t;

#endif