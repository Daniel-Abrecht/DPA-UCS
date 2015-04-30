#include <stdio.h>
#include <protocol/icmp.h>

bool DPAUCS_icmp_handler( void* from, void* to, DPAUCS_beginTransmission begin, DPAUCS_endTransmission end, uint16_t offset, uint16_t length, void* payload, bool last ){
  if(!last)
    return false;
  (void)offset;
  (void)length;
  DPAUCS_icmp_t* icmp = payload;
  switch( icmp->type ){
    case ICMP_ECHO_REQUEST: {
      icmp->type = ICMP_ECHO_REPLY;
      void* ret[] = {from,0};
      stream_t* stream = (*begin)(to,ret,IP_PROTOCOL_ICMP);
      (void)stream;
      (*end)();
    } break;
  }
/*  printf(
    "icmp: %u %u %u %u\n",
    (unsigned)offset,
    (unsigned)length,
    (unsigned)icmp->type,
    (unsigned)icmp->code
  );*/
  return true;
}

