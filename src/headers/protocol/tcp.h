#ifndef TCP_H
#define TCP_H

#define IP_PROTOCOL_TCP 6

#include <stdbool.h>
#include <packed.h>
#include <protocol/ip.h>

typedef PACKED1 struct PACKED2 {
  uint16_t source;
  uint16_t destination;
  uint32_t sequence;
  uint32_t acknowledgment;
  union {
    uint8_t dataOffset; // 5: header length | 3 reserved
    uint16_t flags;
  };
  uint16_t windowSize;
  uint16_t checksum;
  uint16_t urgentPointer;
} DPAUCS_tcp_t;

enum tcp_flags {
  TCP_FLAG_FIN = 1<<0,
  TCP_FLAG_SYN = 1<<1,
  TCP_FLAG_RST = 1<<2,
  TCP_FLAG_PSH = 1<<3,
  TCP_FLAG_ACK = 1<<4,
  TCP_FLAG_URG = 1<<5,
  TCP_FLAG_ECE = 1<<6,
  TCP_FLAG_CWR = 1<<7,
  TCP_FLAG_NS  = 1<<8
};

void DPAUCS_tcpInit();
void DPAUCS_tcpShutdown();

#endif
