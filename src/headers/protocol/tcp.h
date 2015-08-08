#ifndef TCP_H
#define TCP_H

#define PROTOCOL_TCP 6

#include <stdbool.h>
#include <helper_macros.h>
#include <protocol/layer3.h>
#include <service.h>

DPAUCS_MODUL( tcp );

#ifndef TRANSMISSION_CONTROL_BLOCK_COUNT
#define TRANSMISSION_CONTROL_BLOCK_COUNT (1<<5)
#endif 

#define TCP_ACK_TIMEOUT 250
#define TCP_KEEPALIVE_CHECK 6 * 1000
#define TCP_DROP_AFTER_FAILTURES 10

#define DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE 1460

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

#define TCP_STATES(X) \
  X(TCP_CLOSED_STATE), \
  X(TCP_SYN_RCVD_STATE), \
  X(TCP_SYN_SENT_STATE), \
  X(TCP_ESTAB_STATE), \
  X(TCP_FIN_WAIT_1_STATE), \
  X(TCP_CLOSE_WAIT_STATE), \
  X(TCP_FIN_WAIT_2_STATE), \
  X(TCP_CLOSING_STATE), \
  X(TCP_LAST_ACK_STATE), \
  X(TCP_TIME_WAIT_STATE)

#define DPAUCS_EVAL(X) X

typedef enum {
  TCP_STATES(DPAUCS_EVAL)
} TCP_state_t;

typedef struct DPAUCS_tcp_fragment DPAUCS_tcp_fragment_t;

typedef struct transmissionControlBlock {

  // TCP stuff //

  TCP_state_t state;

  struct {
    uint32_t
      UNA,
      NXT,
      WND
    ;
  } SND;

  struct {
    uint32_t
      NXT,
      WND
    ;
  } RCV;

  DPAUCS_address_pair_t fromTo;
  uint16_t srcPort, destPort;

  // internal stuff //
  DPAUCS_service_t* service;
  struct {
    DPAUCS_tcp_fragment_t **first, **last;
  } fragments;
  void* currentId;
  uint16_t next_length, checksum;

  struct {
    bool ackAlreadySent : 1;
  } flags;

  ////////////////////

} transmissionControlBlock_t;

typedef struct {
  uint32_t
    SEQ,
    LEN
  ;
  uint16_t flags;
} tcp_segment_t;

typedef struct {
  DPAUCS_tcp_t* tcp;
  DPAUCS_stream_t* stream;
} DPAUCS_tcp_transmission_t;

void WEAK DPAUCS_tcpInit();
void WEAK DPAUCS_tcpShutdown();
void WEAK tcp_do_next_task( void );

bool DPAUCS_tcp_send( bool(*func)( DPAUCS_stream_t* stream, void* ptr ), void** cids, size_t count, void* ptr );
void DPAUCS_tcp_close( void* cid );
void DPAUCS_tcp_abord( void* cid );

#endif
