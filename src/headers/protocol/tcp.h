#ifndef TCP_H
#define TCP_H

#define PROTOCOL_TCP 6

#include <stdbool.h>
#include <packed.h>
#include <protocol/layer3.h>

#ifndef TEMPORARY_TRANSMISSION_CONTROL_BLOCK_COUNT
#define TEMPORARY_TRANSMISSION_CONTROL_BLOCK_COUNT (1<<3)
#endif 

#ifndef STATIC_TRANSMISSION_CONTROL_BLOCK_COUNT
#define STATIC_TRANSMISSION_CONTROL_BLOCK_COUNT 32
#endif

#define TCP_ACK_TIMEOUT 250
#define TCP_KEEPALIVE_CHECK 6 * 1000
#define TCP_DROP_AFTER_FAILTURES 10


typedef PACKED1 struct PACKED2 {
  uint16_t source;
  uint16_t destination;
  uint32_t sequence;
  uint32_t acknowledgment;
  union {
    uint8_t dataOffset; // 4: header length | 3 reserved
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

typedef enum {
  TCP_SYN_RCVD_STATE,
  TCP_SYN_SENT_STATE,
  TCP_ESTAB_STATE,
  TCP_FIN_WAIT_1_STATE,
  TCP_CLOSE_WAIT_STATE,
  TCP_FIN_WAIT_2_STATE,
  TCP_CLOSING_STATE,
  TCP_LAST_ACK_STATE,
  TCP_TIME_WAIT_STATE
} TCP_state_t;

typedef struct {

  // internal stuff //
  bool active;
  uint16_t srcPort, destPort;
  DPAUCS_address_t *srcAddr, *destAddr;
  DPAUCS_service_t* service;

  // TCP stuff //
  
  TCP_state_t state;

  ///////////////
  
} transmissionControlBlock_t;

void DPAUCS_tcpInit();
void DPAUCS_tcpShutdown();

#endif
