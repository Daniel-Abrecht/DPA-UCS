#include <stdbool.h>
#include <string.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/server.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/ethtypes.h>
#include <DPA/UCS/protocol/anyAddress.h>

typedef struct {
  uint8_t referenceCount;
  // Enougth memory for any kind of address //
  DPAUCS_address_t address;
  char followingAddressMemory[sizeof(DPAUCS_any_logicAddress_t)-sizeof(DPAUCS_logicAddress_t)];
  ////
} ARP_entry_t;

ARP_entry_t entries[ ARP_ENTRY_COUNT ];

static inline ARP_entry_t* arpCache_getEntryByAddress( const DPAUCS_logicAddress_t* addr ){
  ARP_entry_t* entry = (void*)( ((char*)addr) - offsetof(DPAUCS_address_t,logicAddress) - offsetof(ARP_entry_t,address) );

  if( entry >= entries && entry < entries + ARP_ENTRY_COUNT )
    return entry;

  for(
    ARP_entry_t* it = entries;
    it < entries + ARP_ENTRY_COUNT;
    it++
  ) if( 
    DPAUCS_compare_logicAddress( 
      &it->address.logicAddress,
      addr
    )
  ) return it;

  return 0;
}

const DPAUCS_address_t* DPAUCS_arpCache_register( const DPAUCS_address_t* addr ){

  ARP_entry_t* entry = arpCache_getEntryByAddress( &addr->logicAddress );

  if( !entry ){

    for(
      ARP_entry_t* it = entries;
      it < entries + ARP_ENTRY_COUNT;
      it++
    ) if(!it->referenceCount)
      entry = it;

    if(!entry)
      return 0;

    entry->address = *addr;
    DPAUCS_copy_logicAddress( &entry->address.logicAddress, &addr->logicAddress );

  }

  if( entry->referenceCount == 0xFFu )
    return 0;

  entry->referenceCount++;

  return &entry->address;
}

bool DPAUCS_arpCache_deregister( const DPAUCS_logicAddress_t* addr ){
  ARP_entry_t* entry = arpCache_getEntryByAddress(addr);
  if(!entry)
    return false;
  entry->referenceCount--;
  return true;
}

DPAUCS_address_t* DPAUCS_arpCache_getAddress( const DPAUCS_logicAddress_t* la ){
  ARP_entry_t* entry = arpCache_getEntryByAddress( la );
  if(!entry)
    return 0;
  return &entry->address;
}

void DPAUCS_arp_handler( DPAUCS_packet_info_t* info ){

  DPAUCS_arp_t* arp = info->payload;

  uint8_t 
    *sha = (uint8_t*)info->payload + sizeof(DPAUCS_arp_t), // Sender hardware address
    *spa = sha + arp->hlen, // Sender protocol address
    *tha = spa + arp->plen, // Target hardware address
    *tpa = tha + arp->hlen  // Target protocol address
  ;

  (void)sha;
  (void)spa;
  (void)tha;
  (void)tpa;

  // handle only long enough hardware adresses
  if( arp->hlen != sizeof(DPAUCS_mac_t) )
    return;

  switch( DPA_btoh16( arp->ptype ) ){
#ifdef USE_IPv4
    case ETH_TYPE_IP_V4: {
      // IPv4 addresses must be 4 bytes long
      if( arp->plen != 4 ) return;

      uint32_t src_ip  = DPA_btoh32( *(uint32_t*)spa );
      uint32_t dest_ip = DPA_btoh32( *(uint32_t*)tpa );

      if( dest_ip ){
        DPAUCS_logicAddress_IPv4_t addr = {
          DPAUCS_LA_IPv4_INIT,
          .address = dest_ip
        };
        if( !DPAUCS_isValidHostAddress(&addr.logicAddress)
         || !DPAUCS_has_logicAddress(&addr.logicAddress)
        ) return;
      }

      switch( DPA_btoh16( arp->oper ) ){
        case ARP_REQUEST: { // request recived, make a response

          DPAUCS_packet_info_t infReply = *info;
          // set destination mac of ethernet frame to source mac of recived frame 
          memcpy(infReply.destination_mac,info->source_mac,sizeof(DPAUCS_mac_t));

          // Fill in source mac and payload pointing to new buffer
          // Initializes ethernet frame
          DPAUCS_preparePacket(&infReply);

          DPAUCS_arp_t* rarp = infReply.payload;
          *rarp = *arp; // preserve htype, ptype, etc.

          rarp->oper = DPA_htob16( ARP_RESPONSE );

          uint8_t 
            *rsha = (uint8_t*)infReply.payload + sizeof(DPAUCS_arp_t), // Sender hardware address
            *rspa = rsha + rarp->hlen, // Sender protocol address
            *rtha = rspa + rarp->plen, // Target hardware address
            *rtpa = rtha + rarp->hlen  // Target protocol address
          ;

          memcpy(rsha,info->interface->mac,sizeof(DPAUCS_mac_t)); // source is my mac
          *(uint32_t*)rspa = DPA_htob32(dest_ip); // my source mac is the previous target ip
          memcpy(rtha,sha,sizeof(DPAUCS_mac_t)); // target mac is previous source mac
          *(uint32_t*)rtpa = DPA_htob32(src_ip); // target ip is previous source ip

          // send ethernet frame
          DPAUCS_sendPacket(
            &infReply,
            sizeof(DPAUCS_arp_t) + 2*sizeof(DPAUCS_mac_t) + 8 // 8: 2*IPv4
          );

        } break;
        case ARP_RESPONSE: {
          // May be usful later
        } break;
      }

    } break;
#endif
  }

}

