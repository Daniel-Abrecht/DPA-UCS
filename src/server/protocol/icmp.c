#include <stdio.h>
#include <protocol/icmp.h>

bool DPAUCS_icmp_handler(void* id, DPAUCS_sendHandler send, uint16_t offset, uint16_t length, void* payload, bool last){
  if(!last)
    return false;
  (void)offset;
  DPAUCS_icmp_t* icmp = payload;
  switch( icmp->type ){
    case ICMP_ECHO_REQUEST: {
      icmp->type = ICMP_ECHO_REPLY;
      (*send)(id,icmp,length);
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

