#include <string.h>
#include <stdbool.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/server.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/layer3.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/ethtypes.h>

enum arp_operation {
  ARP_OP_REQUEST  = 1,
  ARP_OP_RESPONSE = 2
};

typedef struct {
  uint16_t referenceCount;
  DPAUCS_address_t address;
  char followingAddressMemory[];
} ARP_entry_t;


static char entries[ ARP_ENTRY_BUFFER_SIZE ];
static const ARP_entry_t *entries_end;


static inline size_t getLargestAddressSize( void ){
  static size_t size = 0;
  return size;
}

static inline size_t getRealArpEntrySize( void ){
  return sizeof(ARP_entry_t) - sizeof(DPAUCS_address_t) + getLargestAddressSize();
}

static inline void next_arp_entry( ARP_entry_t** entry ){
  *(char**)entry += getRealArpEntrySize();
}

static inline ARP_entry_t* arpCache_getEntryByAddress( const DPAUCS_logicAddress_t* addr ){
  void* entry = ((char*)addr) - offsetof(DPAUCS_address_t,logicAddress) - offsetof(ARP_entry_t,address);

  if( entry >= (void*)entries && entry < (void*)entries_end )
    return entry;

  for( ARP_entry_t* it = (ARP_entry_t*)entries; it < entries_end; next_arp_entry(&it) )
    if( DPAUCS_compare_logicAddress( &it->address.logicAddress, addr ) )
      return it;

  return 0;
}


const DPAUCS_address_t* DPAUCS_arpCache_register( const DPAUCS_address_t* addr ){

  ARP_entry_t* entry = arpCache_getEntryByAddress( &addr->logicAddress );

  if( !entry ){

  for( ARP_entry_t* it = (ARP_entry_t*)entries; it < entries_end; next_arp_entry(&it) )
    if(!it->referenceCount)
      entry = it;

    if(!entry)
      return 0;

    entry->address = *addr;
    DPAUCS_copy_logicAddress( &entry->address.logicAddress, &addr->logicAddress );

  }

  if( entry->referenceCount == 0xFFFFu )
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


void checkIfMyAddress( const DPAUCS_logicAddress_t* address, void* param ){
  bool* result = param;
  *result = DPAUCS_isValidHostAddress(address)
         && DPAUCS_has_logicAddress(address);
}


static void packetHandler( DPAUCS_packet_info_t* info ){

  DPAUCS_arp_t* arp = info->payload;

  char
    *sha = (char*)info->payload + sizeof(DPAUCS_arp_t), // Sender hardware address
    *spa = sha + arp->hlen, // Sender protocol address
    *tha = spa + arp->plen, // Target hardware address
    *tpa = tha + arp->hlen  // Target protocol address
  ;

  // handle only long enough hardware adresses
  if( arp->hlen != sizeof(DPAUCS_mac_t) )
    return;

  bool result = false;
  DPAUCS_withRawAsLogicAddress( DPA_btoh16( arp->ptype ), tpa, arp->plen, checkIfMyAddress, &result );
  if( !result ) return;

  switch( (enum arp_operation)DPA_btoh16( arp->oper ) ){
    case ARP_OP_REQUEST: { // request received, make a response

      DPAUCS_packet_info_t infReply = *info;
      // set destination mac of ethernet frame to source mac of received frame 
      memcpy(infReply.destination_mac,info->source_mac,sizeof(DPAUCS_mac_t));

      // Fill in source mac and payload pointing to new buffer
      // Initializes ethernet frame
      DPAUCS_preparePacket(&infReply);

      DPAUCS_arp_t* rarp = infReply.payload;
      *rarp = *arp; // preserve htype, ptype, etc.

      rarp->oper = DPA_htob16( ARP_OP_RESPONSE );

      char
        *rsha = (char*)infReply.payload + sizeof(DPAUCS_arp_t), // Sender hardware address
        *rspa = rsha + rarp->hlen, // Sender protocol address
        *rtha = rspa + rarp->plen, // Target hardware address
        *rtpa = rtha + rarp->hlen  // Target protocol address
      ;

      memcpy(rsha,info->interface->mac,sizeof(DPAUCS_mac_t)); // source is my mac
      memcpy(rspa,tpa,arp->plen); // my source mac is the previous target ip
      memcpy(rtha,sha,sizeof(DPAUCS_mac_t)); // target mac is previous source mac
      memcpy(rtpa,spa,arp->plen); // target ip is previous source ip

      // send ethernet frame
      DPAUCS_sendPacket(
        &infReply,
        sizeof(DPAUCS_arp_t) + 2*sizeof(DPAUCS_mac_t) + 8 // 8: 2*IPv4
      );

    } break;
    case ARP_OP_RESPONSE: {
      // May be usful later
    } break;
  }

}

static const DPAUCS_l3_handler_t handler = {
  .type = DPAUCS_ETH_T_ARP,
  .packetHandler = &packetHandler
};

DPAUCS_EXPORT_L3_HANDLER( ARP, &handler );

