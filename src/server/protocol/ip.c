#include <DPA/UCS/packet.h>
#include <DPA/UCS/protocol/ip.h>
#include <DPA/UCS/protocol/IPv4.h>

void DPAUCS_ip_handler( DPAUCS_packet_info_t* info ){
  switch( (*(uint8_t*)info->payload) >> 4 ){
#ifdef USE_IPv4
    case 4: DPAUCS_IPv4_handler(info,info->payload); break;
#endif
  }
}

