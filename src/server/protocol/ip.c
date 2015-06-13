#include <protocol/ip.h>

void DPAUCS_ip_handler( DPAUCS_packet_info* info ){
  DPAUCS_ip_t* ip = info->payload;
  switch( ip->version >> 4 ){
    case 4: DPAUCS_IPv4_handler(info,&ip->IPv4); break;
  }
}

