#define DPAUCS_DHCP_C

#include <stdint.h>
#include <DPA/UCS/server.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/protocol/udp.h>
#include <DPA/UCS/services/dhcp.h>
#include <DPA/UCS/driver/ethernet.h>

DPA_MODULE( dhcp ){
  DPA_DEPENDENCY( udp );
}

typedef struct packed {
  uint8_t op, htype, hlen, hops;
  uint32_t xid;
  uint16_t secs, flags;
  uint32_t ciaddr;
  uint32_t yiaddr;
  uint32_t siaddr;
  uint32_t giaddr;
  uint8_t chaddr[16];
  char sname[64];
  char file[128];
} DPAUCS_dhcp_t;

static void onreceive( void* request, void* data, size_t size ){
  if( size < sizeof(DPAUCS_dhcp_t) )
    return;
  DPAUCS_dhcp_t* dhcp = data;
  (void)dhcp;
  DPAUCS_udp_descriptor_t reply;
  DPAUCS_udp_make_reply_descriptor(request,&reply);
  (void)data;
  (void)size;
  DPA_LOG("DHCP packet op: %hhu\n",dhcp->op);
}

static void dhcp_discover( DPAUCS_interface_t* interface ){
  (void)interface;
}

static void dhcp_release( DPAUCS_interface_t* interface ){
  (void)interface;
}

void DPAUCS_dhcp_update_interface( DPAUCS_interface_t* interface ){
  if( DPAUCS_is_interface_using_dhcp && !DPAUCS_is_interface_using_dhcp(interface) )
    return;
  if( interface->link_up ){
    dhcp_discover( interface );
  }else{
    dhcp_release( interface );
  }
}

static void interface_event_handler( DPAUCS_interface_t* interface, enum DPAUCS_interface_event type, void* param ){
  (void)param;
  if( type == DPAUCS_IFACE_EVENT_LINK_UP || DPAUCS_IFACE_EVENT_LINK_DOWN )
    DPAUCS_dhcp_update_interface( interface );
}

DPAUCS_INIT {
  if( !DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 68, &dhcp_service ) ){
    static const flash char message[] = {"Failed to add DHCP service!\n"};
    DPAUCS_fatal(message);
  }
}

DPAUCS_SHUTDOWN {
  DPAUCS_remove_service( DPAUCS_ANY_ADDRESS, 68 );
}

DPA_LOOSE_LIST_ADD( DPAUCS_event_list, &interface_event_handler )

const flash DPAUCS_service_t dhcp_service = {
  .tos = PROTOCOL_UDP,
  .onreceive = &onreceive
};
