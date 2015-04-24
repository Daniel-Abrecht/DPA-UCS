#include <stdio.h>
#include <server.h>
#include <binaryUtils.h>
#include <protocol/ip.h>
#include <protocol/ip_stack.h>

static struct {
  uint8_t protocol;
  DPAUCS_ipProtocolHandler_func handler;
} ipProtocolHandlers[MAX_IP_PROTO_HANDLERS];

void DPAUCS_addIpProtocolHandler(uint8_t protocol, DPAUCS_ipProtocolHandler_func handler){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( !ipProtocolHandlers[i].handler ){
      ipProtocolHandlers[i].protocol = protocol;
      ipProtocolHandlers[i].handler = handler;
      break;
    }
}

void DPAUCS_removeIpProtocolHandler(uint8_t protocol){
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( ipProtocolHandlers[i].protocol == protocol ){
      ipProtocolHandlers[i].handler = 0;
      break;
    }
}

static void IPv4_handler( DPAUCS_packet_info* info, DPAUCS_ipv4_t* ip ){
  uint32_t source = htob32(ip->destination);
  uint32_t destination = htob32(ip->destination);

  if(~destination){ // not broadcast
    int i=0;
    for(i=MAX_IPS;i--;) // but one of my ips
      if(ips[i]==destination)
        break;
    if(!destination||i<0) return; // not my ip and not broadcast
  }

  DPAUCS_ipProtocolHandler_func handler = 0;
  for(int i=0; i<MAX_IP_PROTO_HANDLERS; i++)
    if( 
        ipProtocolHandlers[i].handler
     && ipProtocolHandlers[i].protocol == ip->protocol
    ){
      handler = ipProtocolHandlers[i].handler;
      break;
    }
  if(!handler) return; 

  printf(
    "packet src: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
    " | dest: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
    " | type: 0x%.4lx\n"
    "ip src: %u.%u.%u.%u | "
    "ip dest: %u.%u.%u.%u\n",

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

    (long)info->type,

    ( source >> 24 ) & 0xFF,
    ( source >> 16 ) & 0xFF,
    ( source >>  8 ) & 0xFF,
    ( source >>  0 ) & 0xFF,

    ( destination >> 24 ) & 0xFF,
    ( destination >> 16 ) & 0xFF,
    ( destination >>  8 ) & 0xFF,
    ( destination >>  0 ) & 0xFF

  );  

}

void DPAUCS_ip_handler( DPAUCS_packet_info* info ){
  DPAUCS_ip_t* ip = info->payload;
  switch( ip->version >> 4 ){
    case 4: IPv4_handler(info,&ip->ipv4); break;
  }
}

