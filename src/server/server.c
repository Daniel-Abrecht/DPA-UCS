#include <setjmp.h>
#include <string.h>
#include <stdbool.h>
#include <DPA/UCS/eth.h>
#include <DPA/UCS/adelay.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/service.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/binaryUtils.h>
#include <DPA/UCS/helper_macros.h>
#include <DPA/UCS/logger.h>
#include <DPA/UCS/protocol/ethtypes.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/icmp.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/tcp.h>
#include <DPA/UCS/protocol/ip.h>

static void DPAUCS_init( void );
static void DPAUCS_shutdown( void );

const DPAUCS_logicAddress_t* logicAddresses[MAX_LOGIC_ADDRESSES] = {0};

static DPAUCS_packet_t packetInputBuffer;
static DPAUCS_packet_t nextPacketToSend;

static struct {
  bool active;
  const DPAUCS_logicAddress_t* logicAddress;
  uint16_t port;
  DPAUCS_service_t* service;
} services[MAX_SERVICES];

static jmp_buf fatal_error_exitpoint;

void WEAK DPAUCS_onfatalerror( const char* message ){
  DPAUCS_LOG( "%s", message );
}

NORETURN void DPAUCS_fatal( const char* message ){
  DPAUCS_onfatalerror( message );
  longjmp(fatal_error_exitpoint,1);
}

void DPAUCS_run( void(*callback)(void*), void* arg ){
  if(setjmp(fatal_error_exitpoint))
    goto shutdown;

  DPAUCS_init();

  (*callback)(arg);

 shutdown:
  DPAUCS_shutdown();
}

static void DPAUCS_init( void ){

  memset(services,0,sizeof(services));

  DPAUCS_ethInit(mac);

  if(DPAUCS_icmpInit)
    DPAUCS_icmpInit();

#ifdef DPAUCS_INIT // Allows to add init functions using makefile
#define X(F) void F( void ); F();
  DPAUCS_INIT
#undef X
#endif

}

static void DPAUCS_shutdown( void ){

  if(DPAUCS_icmpShutdown)
    DPAUCS_icmpShutdown();

  DPAUCS_ethShutdown();

#ifdef DPAUCS_SHUTDOWN // Allows to add shutdown functions using makefile
#define X(F) void F( void ); F();
  DPAUCS_SHUTDOWN
#undef X
#endif

}

void DPAUCS_add_logicAddress( const DPAUCS_logicAddress_t*const logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if(!logicAddresses[i]){
      logicAddresses[i] = logicAddress;
      break;
    }
  }
}

void DPAUCS_remove_logicAddress( const DPAUCS_logicAddress_t*const logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if(DPAUCS_compare_logicAddress(logicAddresses[i],logicAddress)){
      logicAddresses[i] = 0;
      break;
    }
  }
}

void DPAUCS_each_logicAddress(DPAUCS_address_types_t type, bool(*func)(const DPAUCS_logicAddress_t*,void*),void* x){
  for( int i=0; i<MAX_LOGIC_ADDRESSES; i++ )
    if( logicAddresses[i]->type & type )
      if( !(*func)(logicAddresses[i],x) )
        break;
}

bool DPAUCS_has_logicAddress(const DPAUCS_logicAddress_t* logicAddress){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++)
    if(DPAUCS_compare_logicAddress(logicAddresses[i],logicAddress))
      return true;
  return false;
}

void DPAUCS_add_service( const DPAUCS_logicAddress_t*const logicAddress, uint16_t port, DPAUCS_service_t* service ){
  if(!service)
    return;
  for(int i=0;i<MAX_SERVICES;i++){
    if(!services[i].active){
      services[i].active = true;
      services[i].port = port;
      services[i].logicAddress = logicAddress;
      services[i].service = service;
      if(services[i].service->start)
        (*services[i].service->start)();
      break;
    }
  }
}

void DPAUCS_remove_service( const DPAUCS_logicAddress_t*const logicAddress, uint16_t port ){
  for(int i=0;i<MAX_SERVICES;i++){
    if( services[i].active
       && (
          !services[i].logicAddress
       || DPAUCS_compare_logicAddress( services[i].logicAddress, logicAddress )
     ) && services[i].port == port
    ){
      services[i].active = false;
      if(services[i].service->stop)
        (*services[i].service->stop)();
      break;
    }
  }
}

DPAUCS_service_t* DPAUCS_get_service( const DPAUCS_logicAddress_t*const logicAddress, uint16_t port, uint8_t tos ){
  for(int i=0;i<MAX_SERVICES;i++)
    if( services[i].active
        && ( 
           !services[i].logicAddress
        || DPAUCS_compare_logicAddress( services[i].logicAddress, logicAddress )
      ) && services[i].port == port
        && services[i].service->tos == tos
    ) return services[i].service;
    return 0;
}

void getPacketInfo( DPAUCS_packet_info* info, DPAUCS_packet_t* packet ){

  memset(info,0,sizeof(*info));

  memcpy(info->destination_mac,packet->data.dest,6);
  memcpy(info->source_mac,packet->data.src,6);

  info->is_vlan = packet->data.vlan.tpid == IEEE_802_1Q_TPID_CONST;

  if( info->is_vlan ){ // maybe an IEEE 802.1Q tag
    info->vid     = btoh16(packet->data.vlan.pcp_dei_vid) & 0x0FFF;
    info->type    = btoh16(packet->data.vlan.type);
    info->payload = packet->data.vlan.payload;
    info->llc     = &packet->data.vlan.llc;
    info->size    = btoh16(packet->data.length);
  }else{
    info->vid     = 0;
    info->type    = btoh16(packet->data.type);
    info->payload = packet->data.payload;
    info->llc     = &packet->data.llc;
    info->size    = btoh16(packet->data.vlan.length);
  }

  if( info->type < 1536 && info->type > 1500 ){ // undefined case
    info->invalid = true;
  }

  if( info->type <= 1500 ){ // field contained length instant of type
    if(info->llc->dsap != 0xAA){ // Cant handle this kind of frame
      info->invalid = true;
      return;
    }else{ // ok, it's snap
      info->type    = btoh16(info->llc->extended_control.snap.type);
      info->payload = info->llc->extended_control.payload;
    }
  }

}


void DPAUCS_doNextTask( void ){
  do { // Recive & handle packet

    DPAUCS_packet_t*const packet = &packetInputBuffer;
    packet->size = DPAUCS_ethReceive(packet->data.raw,PACKET_SIZE);

    if(!packet->size)
      break;

    if(
         memcmp(packet->data.dest,"\xFF\xFF\xFF\xFF\xFF\xFF",6) // not broadcast
      && memcmp(packet->data.dest,mac,6) // not my mac
    ) break; // ignore packet

    DPAUCS_packet_info info;

    getPacketInfo(&info,packet);

    if(info.invalid){
      packet->size = 0; // discard package
      break;
    }

    switch(info.type){
      case ETH_TYPE_ARP: DPAUCS_arp_handler(&info); break;
      case ETH_TYPE_IP_V4: DPAUCS_ip_handler(&info); break;
    }

  } while(0);

  if(adelay_update)
    adelay_update();

  if(tcp_do_next_task)
    tcp_do_next_task();

}

void DPAUCS_preparePacket( DPAUCS_packet_info* info ){

  DPAUCS_packet_t* packet = &nextPacketToSend;

  memcpy(info->source_mac,mac,6);
  
  memcpy(packet->data.dest,info->destination_mac,6);
  memcpy(packet->data.src,info->source_mac,6);

  if( info->is_vlan ){
    packet->data.vlan.tpid = IEEE_802_1Q_TPID_CONST;
    packet->data.vlan.pcp_dei_vid = htob16(info->vid);
    packet->data.vlan.type = htob16(info->type);
    info->payload = packet->data.vlan.payload;
  }else{
    packet->data.vlan.pcp_dei_vid  = 0;
    packet->data.type      = htob16(info->type);
    info->payload          = packet->data.payload;
  }

  packet->size = (uintptr_t)packet->data.payload - (uintptr_t)packet;

}

void DPAUCS_sendPacket( DPAUCS_packet_info* info, uint16_t size ){
  size += (uint8_t*)info->payload - nextPacketToSend.data.raw; // add ethernetheadersize
  nextPacketToSend.size = size;
  DPAUCS_ethSend( nextPacketToSend.data.raw, size );
}

