#include <stdio.h>
#include <protocol/ip.h>

void DPAUCS_ipv4_handler( DPAUCS_packet_info* info ){
  printf("packet src: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x | dest: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x | type: 0x%.4lx\n",

    info->source_mac[0],
    info->source_mac[1],
    info->source_mac[2],
    info->source_mac[3],
    info->source_mac[4],
    info->source_mac[5],

    info->destination_mac[0],
    info->destination_mac[1],
    info->destination_mac[2],
    info->destination_mac[3],
    info->destination_mac[4],
    info->destination_mac[5],

    (long)info->type

  );  
}
