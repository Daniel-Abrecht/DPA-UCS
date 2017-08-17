#ifndef DPAUCS_TCP_H
#define DPAUCS_TCP_H

#define PROTOCOL_TCP 6

#include <stdbool.h>
#include <DPA/UCS/adelay.h>
#include <DPA/UCS/service.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/protocol/layer3.h>

DPA_MODULE( tcp );

#ifndef TRANSMISSION_CONTROL_BLOCK_COUNT
#define TRANSMISSION_CONTROL_BLOCK_COUNT (1<<5)
#endif 

#define TCP_ACK_TIMEOUT 250
#define TCP_KEEPALIVE_CHECK 6 * 1000
#define TCP_DROP_AFTER_FAILTURES 10

#define DPAUCS_DEFAULT_RECIVE_WINDOW_SIZE 1460

typedef struct packed DPAUCS_tcp {
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

enum DPAUCS_tcp_flags {
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

enum DPAUCS_TCP_state {
  TCP_STATES(DPAUCS_EVAL)
};

typedef struct DPAUCS_transmissionControlBlock {

  // TCP stuff //

  enum DPAUCS_TCP_state state;

  struct {
    uint32_t
      UNA,
      NXT,
      WND
    ;
  } SND;

  struct {
    uint32_t
      UNA,
      NXT,
      WND
    ;
  } RCV;

  DPAUCS_mixedAddress_pair_t fromTo;
  uint16_t srcPort, destPort;

  // internal stuff //

  void* currentId;
  struct DPAUCS_service* service;

  struct {
    struct DPAUCS_tcp_fragment **first, **last;
  } fragments;

  struct {
    void **first, **last;
    uint32_t first_SEQ; // Sequence number of first segment in cache
    adelay_t last_transmission; // If there was no previous transmission, this must be zero. Otherwise, it mustn't be zero.
    struct { // RST must be handled specially, PUSH and URG are related to the segment in the retransmission cache.
      bool SYN : 1; // SYN not yet acknowledged
      bool FIN : 1; // No more datas will be added to cache
      bool acknowledge_FIN : 1; // ACK for FIN of other endpoint has to be sent
      bool need_ACK : 1; // Need to send an ACK
    } flags;
  } cache;

  uint16_t next_length, checksum;

  ////////////////////

} DPAUCS_transmissionControlBlock_t;

typedef struct DPAUCS_tcp_segment {
  uint32_t
    SEQ,
    LEN
  ;
  uint16_t flags;
} DPAUCS_tcp_segment_t;

typedef struct DPAUCS_tcp_transmission {
  struct DPA_stream* stream;
} DPAUCS_tcp_transmission_t;


extern DPAUCS_transmissionControlBlock_t DPAUCS_transmissionControlBlocks[ TRANSMISSION_CONTROL_BLOCK_COUNT ];

#ifdef DPAUCS_TCP_C
#define W
#else
#define W weak
#endif

void W DPAUCS_tcpInit();
void W DPAUCS_tcpShutdown();
void W tcp_do_next_task( void );

#undef W

bool DPAUCS_tcp_send( bool(*func)( DPA_stream_t* stream, void* ptr ), void** cids, size_t count, void* ptr );
void DPAUCS_tcp_close( void* cid );
void DPAUCS_tcp_abord( void* cid );

bool DPAUCS_tcp_transmit(
  DPA_stream_t* stream,
  DPAUCS_transmissionControlBlock_t* tcb,
  uint16_t flags,
  size_t size,
  uint32_t SEQ
);

#endif
