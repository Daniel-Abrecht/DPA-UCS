#include <stdbool.h>
#include <string.h>
#include <server.h>
#include <binaryUtils.h>
#include <protocol/ethtypes.h>
#include <protocol/arp.h>

void DPAUCS_arp_handler(DPAUCS_packet_info* info){

  DPAUCS_arp_t* arp = info->payload;

  uint8_t 
    *sha = (uint8_t*)info->payload + sizeof(DPAUCS_arp_t), // Sender hardware address
    *spa = sha + arp->hlen, // Sender protocol address
    *tha = spa + arp->plen, // Target hardware address
    *tpa = tha + arp->hlen  // Target protocol address
  ;

  // handle only 6 byte long hardware adresses
  if( arp->hlen != 6 )
    return;

  switch( btoh16( arp->ptype ) ){
    case ETH_TYPE_IP_V4: {
      // IPv4 addresses must be 4 bytes long
      if( arp->plen != 4 ) return;

      uint32_t src_ip  = btoh32( *(uint32_t*)spa );
      uint32_t dest_ip = btoh32( *(uint32_t*)tpa );

      if(dest_ip){ // check if destination isn't broadcast
        int i;
        for(i=MAX_IPS;i--;) // and one of my ips
          if(ips[i]==dest_ip)
            break;
        if(i<0) return; // not my ip and not broadcast
      }

      switch( btoh16( arp->oper ) ){
        case ARP_REQUEST: { // request recived, make a response

          DPAUCS_packet_info infReply = *info;
          // set destination mac of ethernet frame to source mac of recived frame 
          memcpy(infReply.destination_mac,info->source_mac,6);

          // Fill in source mac and payload pointing to new buffer
          // Initializes ethernet frame
          DPAUCS_preparePacket(&infReply);

          DPAUCS_arp_t* rarp = infReply.payload;
          *rarp = *arp; // preserve htype, ptype, etc.

	  rarp->oper = htob16( ARP_RESPONSE );

          uint8_t 
            *rsha = (uint8_t*)infReply.payload + sizeof(DPAUCS_arp_t), // Sender hardware address
            *rspa = rsha + rarp->hlen, // Sender protocol address
            *rtha = rspa + rarp->plen, // Target hardware address
            *rtpa = rtha + rarp->hlen  // Target protocol address
          ;

          memcpy(rsha,mac,6); // source is my mac
          *(uint32_t*)rspa = htob32(dest_ip); // my source mac is the previous target ip
          memcpy(rtha,sha,6); // target mac is previous source mac
          *(uint32_t*)rtpa = htob32(src_ip); // target ip is previous source ip

          // send ethernet frame
          DPAUCS_sendPacket(
            &infReply,
            sizeof(DPAUCS_arp_t) + 8 + 12 // 8: 2*IPv4 | 12: 2*MAC
          );

        } break;
        case ARP_RESPONSE: {
          // May be usful later
        } break;
      }

    } break;
    default: return; // Unsupportet protocol type
  }

}

