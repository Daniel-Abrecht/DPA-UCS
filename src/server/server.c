#include <setjmp.h>
#include <string.h>
#include <stdbool.h>
#include <DPA/UCS/server.h>
#include <DPA/UCS/packet.h>
#include <DPA/UCS/service.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/utils/helper_macros.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/icmp.h>
#include <DPA/UCS/protocol/address.h>
#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/driver/adelay.h>
#include <DPA/UCS/driver/ethernet.h>

static void DPAUCS_init( void );
static void DPAUCS_shutdown( void );

typedef struct DPAUCS_address_entry {
  const DPAUCS_logicAddress_t* logicAddress;
  const DPAUCS_interface_t* interface;
} DPAUCS_address_entry_t;

DPAUCS_address_entry_t address_list[MAX_LOGIC_ADDRESSES];

static DPAUCS_packet_t packetInputBuffer;
static DPAUCS_packet_t nextPacketToSend;

static struct {
  bool active;
  const DPAUCS_logicAddress_t* logicAddress;
  uint16_t port;
  DPAUCS_service_t* service;
} services[MAX_SERVICES];

static jmp_buf fatal_error_exitpoint;

void weak DPAUCS_onfatalerror( const flash char* message ){
  (void)message;
  DPA_LOG( "%"PRIsFLASH, message );
}

noreturn void DPAUCS_fatal( const flash char* message ){
  DPAUCS_onfatalerror( message );
  longjmp(fatal_error_exitpoint,1);
}

void DPAUCS_run( void(*callback)(void*), void* arg ){
  if(!setjmp(fatal_error_exitpoint)){
    DPAUCS_init();
    (*callback)(arg);
  }
  DPAUCS_shutdown();
}

static void DPAUCS_init( void ){

  memset(services,0,sizeof(services));

  DPA_LOG("Initialising DPAUCS...\n");

  for( struct DPAUCS_init_function_list* it = DPAUCS_init_function_list; it; it = it->next ){
    (*it->entry)();
  }

  for( struct DPAUCS_ethernet_driver_list* it = DPAUCS_ethernet_driver_list; it; it = it->next ){
    DPA_LOG("Initialize ethernet driver \"%s\"\n",it->entry->name);
    (*it->entry->init)();
  }

  DPA_LOG("Initialisation done\n");

}

static void DPAUCS_shutdown( void ){

  DPA_LOG("Shutdown of DPAUCS...\n");

  for( struct DPAUCS_shutdown_function_list* it = DPAUCS_shutdown_function_list; it; it = it->next ){
    (*it->entry)();
  }

  for( struct DPAUCS_ethernet_driver_list* it = DPAUCS_ethernet_driver_list; it; it = it->next ){
    DPA_LOG("Shutdown of ethernet driver \"%s\"\n",it->entry->name);
    (*it->entry->shutdown)();
  }

  DPA_LOG("Shutdown completed...\n");

}

void DPAUCS_add_logicAddress( const DPAUCS_interface_t*const interface, const DPAUCS_logicAddress_t*const logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if(!address_list[i].logicAddress){
      address_list[i].logicAddress = logicAddress;
      address_list[i].interface    = interface;
      break;
    }
  }
}

void DPAUCS_remove_logicAddress( const DPAUCS_logicAddress_t*const logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if( address_list[i].logicAddress && DPAUCS_compare_logicAddress(address_list[i].logicAddress,logicAddress) ){
      address_list[i].logicAddress = 0;
      break;
    }
  }
}

void DPAUCS_each_logicAddress(uint16_t type, bool(*func)(const DPAUCS_logicAddress_t*,void*),void* x){
  for( int i=0; i<MAX_LOGIC_ADDRESSES; i++ )
    if( address_list[i].logicAddress && ( address_list[i].logicAddress->type & type ) )
      if( !(*func)(address_list[i].logicAddress,x) )
        break;
}

bool DPAUCS_has_logicAddress(const DPAUCS_logicAddress_t* logicAddress){
  return DPAUCS_get_logicAddress(logicAddress);
}

const DPAUCS_logicAddress_t* DPAUCS_get_logicAddress( const DPAUCS_logicAddress_t* logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if( address_list[i].logicAddress
     && DPAUCS_compare_logicAddress(address_list[i].logicAddress,logicAddress)
    ) return address_list[i].logicAddress;
  }
  return 0;
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

void getPacketInfo( const DPAUCS_interface_t* interface, DPAUCS_packet_info_t* info, DPAUCS_packet_t* packet ){

  memset(info,0,sizeof(*info));

  info->interface = interface;

  memcpy(info->destination_mac,packet->data.dest,sizeof(DPAUCS_mac_t));
  memcpy(info->source_mac,packet->data.src,sizeof(DPAUCS_mac_t));

  info->is_vlan = packet->data.vlan.tpid == IEEE_802_1Q_TPID_CONST;

  if( info->is_vlan ){ // maybe an IEEE 802.1Q tag
    info->vid     = DPA_btoh16(packet->data.vlan.pcp_dei_vid) & 0x0FFF;
    info->type    = DPA_btoh16(packet->data.vlan.type);
    info->payload = packet->data.vlan.payload;
    info->llc     = &packet->data.vlan.llc;
    info->size    = DPA_btoh16(packet->data.length);
  }else{
    info->vid     = 0;
    info->type    = DPA_btoh16(packet->data.type);
    info->payload = packet->data.payload;
    info->llc     = &packet->data.llc;
    info->size    = DPA_btoh16(packet->data.vlan.length);
  }

  if( info->type < 1536 && info->type > 1500 ){ // undefined case
    info->invalid = true;
  }

  if( info->type <= 1500 ){ // field contained length instant of type
    if(info->llc->dsap != 0xAA){ // Cant handle this kind of frame
      info->invalid = true;
      return;
    }else{ // ok, it's snap
      info->type    = DPA_btoh16(info->llc->extended_control.snap.type);
      info->payload = info->llc->extended_control.payload;
    }
  }

}

typedef struct receive_driver_state {
  struct DPAUCS_ethernet_driver_list* driver;
  struct DPAUCS_interface_list* interface;
} receive_driver_state_t;
static receive_driver_state_t current_receive_driver_state;

const DPAUCS_interface_t* DPAUCS_getInterface( const DPAUCS_logicAddress_t* logicAddress ){
  for(int i=0;i<MAX_LOGIC_ADDRESSES;i++){
    if( address_list[i].logicAddress && DPAUCS_compare_logicAddress(address_list[i].logicAddress,logicAddress) )
      return address_list[i].interface;
  }
  return 0;
}

const DPAUCS_ethernet_driver_t* getDriverOfInterface( const DPAUCS_interface_t* interface ){
  for( struct DPAUCS_ethernet_driver_list* i = DPAUCS_ethernet_driver_list; i; i = i->next )
    for( struct DPAUCS_interface_list* j = i->entry->interface_list; j; j = j->next )
      if( interface == j->entry )
        return i->entry;
  return 0;
}

static bool DPAUCS_receive_next_interface( void ){
  if( !DPAUCS_ethernet_driver_list )
    return false;
  if( !current_receive_driver_state.driver || !current_receive_driver_state.driver->next ){
    current_receive_driver_state.driver = DPAUCS_ethernet_driver_list;
    current_receive_driver_state.interface = current_receive_driver_state.driver->entry->interface_list;
  }else if( !current_receive_driver_state.interface->next ){
    current_receive_driver_state.driver = current_receive_driver_state.driver->next;
    current_receive_driver_state.interface = current_receive_driver_state.driver->entry->interface_list;
  }else{
    current_receive_driver_state.interface = current_receive_driver_state.interface->next;
  }
  return !!current_receive_driver_state.interface;
}

static void DPAUCS_receive_next(){

  DPAUCS_packet_t*const packet = &packetInputBuffer;

  if( !DPAUCS_receive_next_interface() )
    return;

  packet->size = (*current_receive_driver_state.driver->entry->receive)(
    current_receive_driver_state.interface->entry,
    packet->data.raw,
    PACKET_SIZE
  );

  if(!packet->size)
    return;

  if( memcmp(packet->data.dest,"\xFF\xFF\xFF\xFF\xFF\xFF",sizeof(DPAUCS_mac_t)) // not broadcast
   && memcmp(packet->data.dest,current_receive_driver_state.interface->entry->mac,sizeof(DPAUCS_mac_t)) // not my mac
  ) return; // ignore packet

  DPAUCS_packet_info_t info;

  getPacketInfo(
    current_receive_driver_state.interface->entry,
    &info, packet
  );

  if(info.invalid){
    packet->size = 0; // discard package
    return;
  }

  DPAUCS_layer3_packetHandler(&info);

}

void DPAUCS_doNextTask( void ){

  DPAUCS_receive_next();

  if(adelay_update)
    adelay_update();

  if(tcp_do_next_task)
    tcp_do_next_task();

}

void DPAUCS_preparePacket( DPAUCS_packet_info_t* info ){

  DPAUCS_packet_t* packet = &nextPacketToSend;

  memcpy(packet->data.dest,info->destination_mac,sizeof(DPAUCS_mac_t));
  memcpy(packet->data.src,info->source_mac,sizeof(DPAUCS_mac_t));

  if( info->is_vlan ){
    packet->data.vlan.tpid = IEEE_802_1Q_TPID_CONST;
    packet->data.vlan.pcp_dei_vid = DPA_htob16(info->vid);
    packet->data.vlan.type = DPA_htob16(info->type);
    info->payload = packet->data.vlan.payload;
  }else{
    packet->data.vlan.pcp_dei_vid  = 0;
    packet->data.type      = DPA_htob16(info->type);
    info->payload          = packet->data.payload;
  }

  packet->size = (uintptr_t)packet->data.payload - (uintptr_t)packet;

}

void DPAUCS_sendPacket( DPAUCS_packet_info_t* info, uint16_t size ){
  if( !info->interface ){
    DPA_LOG("DPAUCS_sendPacket: Missing interface!\n");
    return;
  }
  size += (uint8_t*)info->payload - nextPacketToSend.data.raw; // add ethernetheadersize
  nextPacketToSend.size = size;
  const DPAUCS_ethernet_driver_t* driver = getDriverOfInterface( info->interface );
  if(!driver){
    DPA_LOG("DPAUCS_sendPacket: interface or driver unavailable\n");
    return;
  }
  (*driver->send)( info->interface, nextPacketToSend.data.raw, size );
}

