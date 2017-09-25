#define DPAUCS_DHCP_C

#include <stdint.h>
#include <string.h>
#include <DPA/UCS/server.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/utils/stream.h>
#include <DPA/UCS/protocol/udp.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/layer3.h>
#include <DPA/UCS/services/dhcp.h>
#include <DPA/UCS/services/dhcp_client.h>
#include <DPA/UCS/driver/adelay.h>
#include <DPA/UCS/driver/ethernet.h>

DPA_MODULE( dhcp_client ){
  DPA_DEPENDENCY( udp );
}

enum dhcp_state {
  DHCP_STATE_NONE,
  DHCP_STATE_DISCOVER_SENT,
  DHCP_STATE_REQUEST_SENT,
  DHCP_STATE_ACCEPTED
};

typedef struct dhcp_lease {
  enum dhcp_state state;
  const DPAUCS_interface_t* interface;
  DPAUCS_IPv4_logicAddress_t ip;
  adelay_t lease_start;
} dhcp_lease_t;

dhcp_lease_t leases[DPAUCS_DHCP_CLIENT_MAX_LEASES];

static dhcp_lease_t* dhcp_alloc_lease( void ){
  for( size_t i=0; i<DPAUCS_DHCP_CLIENT_MAX_LEASES; i++ )
    if( leases[i].state == DHCP_STATE_NONE ){
      leases[i].ip.real.type = DPAUCS_ETH_T_IPv4;
      adelay_start( &leases[i].lease_start );
      return leases + i;
    }
  return 0;
}

static dhcp_lease_t* dhcp_get_lease( const DPAUCS_interface_t* interface ){
  for( size_t i=0; i<DPAUCS_DHCP_CLIENT_MAX_LEASES; i++ )
    if( leases[i].state != DHCP_STATE_NONE
     && leases[i].interface == interface
    ) return leases + i;
  return 0;
}

static void dhcp_remove_lease( const DPAUCS_interface_t* interface ){
  dhcp_lease_t* lease = dhcp_get_lease( interface );
  if( !lease )
    return;
  if( lease->state == DHCP_STATE_ACCEPTED )
    DPAUCS_remove_logicAddress( &lease->ip.super );
  lease->state = DHCP_STATE_NONE;
}

static void dhcp_request( const DPAUCS_interface_t*, const uint8_t[4], const uint8_t[4] );
static void dhcp_discover( const DPAUCS_interface_t* );

#define OPTION_LOOP(OP,DATA,LEN,X) \
  do { \
    uint16_t _s = size; \
    uint8_t* DATA = options; \
    while( _s-- ){ \
      enum dhcp_options OP = *(DATA++); \
      if( OP == 0x00 ) continue; \
      if( OP == 0xFF ) break; \
      if(!(_s--)) break; \
      {  \
        uint8_t LEN = *(DATA++); \
        if( _s < LEN ) return; \
        { \
          DPA_UNPACK X \
        } \
        _s -= LEN; \
        DATA += LEN; \
      } \
    } \
  } while(0)
static void onreceive( void* request, void* data, size_t size ){
  if( size < sizeof(DPAUCS_dhcp_t) + 200 )
    return;
  DPAUCS_dhcp_t* dhcp = data;
  if( dhcp->op != DHCP_OP_BOOT_REPLY )
    return;
  const DPAUCS_udp_descriptor_t* req = request;
  const DPAUCS_address_t* own_addr = DPAUCS_mixedPairComponentToAddress( &req->fromTo, false );
  if( !own_addr ){
    DPA_LOG("Failed to obtain own address\n");
    return;
  }
  const DPAUCS_interface_t* interface = DPAUCS_getInterfaceByPhysicalAddress(own_addr->mac);
  if( !interface ){
    DPA_LOG("Couldn't get interface\n");
    return;
  }
  dhcp_lease_t* lease = dhcp_get_lease( interface );
  if( !lease || lease->lease_start != dhcp->xid )
    return;
  uint8_t* options = (uint8_t*)data + sizeof(DPAUCS_dhcp_t) + 192;
  if( memcmp(options, (uint8_t[]){DHCP_OPTION_MAGIC}, 4 ) )
    return;
  options += 4;
  if( *(options++) != DHCP_OPTION_MESSAGE_TYPE || *(options++) != 1 )
    return;
  enum dhcp_message_types message_type = *(options++);
  size -= sizeof(DPAUCS_dhcp_t) - 192 - 7;
  if( message_type == DHCP_MESSAGE_TYPE_DHCPOFFER ){
    if( lease->state != DHCP_STATE_DISCOVER_SENT )
      return;
    uint8_t* server_ip = 0;
    OPTION_LOOP( op, data, len, (
      if( op == DHCP_OPTION_SERVER_IDENTIFIER ){
        server_ip = data;
        break;
      }
    ));
    if( !server_ip ){
      DPA_LOG("DHCP-client: required next server identifier missing\n");
    }else{
      dhcp_request( interface, server_ip, dhcp->yiaddr );
    }
  }else if( message_type == DHCP_MESSAGE_TYPE_DHCPNAK ){
    DPA_LOG("DHCPNAK: DHCP request wasn't not accepted\n");
    dhcp_remove_lease(interface);
  }else if( message_type == DHCP_MESSAGE_TYPE_DHCPDECLINE ){
    DPA_LOG("DHCPDECLINE: Requested address may already be in use, retrying...\n");
    dhcp_remove_lease(interface);
    dhcp_discover( interface );
  }else if( message_type == DHCP_MESSAGE_TYPE_DHCPACK ){
    if( lease->state != DHCP_STATE_REQUEST_SENT )
      return;
    lease->state = DHCP_STATE_ACCEPTED;
    memcpy( lease->ip.real.address, dhcp->yiaddr, 4 );
    const uint8_t *subnet=0, *router=0;
    OPTION_LOOP( op, data, len, (
      switch( op ){
        case DHCP_OPTION_SUBNET_MASK: {
          if( len == 4 )
            subnet = data;
        } break;
        case DHCP_OPTION_ROUTER: {
          if( len == 4 )
            router = data;
        } break;
        case DHCP_OPTION_DOMAIN_NAME_SERVER: {
          
        } break;
        case DHCP_OPTION_DOMAIN_NAME: {
          
        } break;
        case DHCP_OPTION_DOMAIN_SEARCH: {
          
        } break;
        case DHCP_OPTION_CLASSLESS_STATIC_ROUTE: {
          
        } break;
        default: break;
      }
    ));
    DPA_LOG("Got ip %hhd.%hhd.%hhd.%hhd\n",lease->ip.real.address[0],lease->ip.real.address[1],lease->ip.real.address[2],lease->ip.real.address[3]);
    DPAUCS_add_logicAddress( interface, &lease->ip.super );
    (void)subnet;
    (void)router;
  }else return;
}
#undef OPTION_LOOP

static void dhcp_request( const DPAUCS_interface_t* interface, const uint8_t sip[4], const uint8_t rip[4] ){
  dhcp_lease_t* lease = dhcp_get_lease(interface);
  if( !lease ){
    DPA_LOG("A DHCP discover has to be sent before the DHCP request\n");
    return;
  }
  DPAUCS_udp_descriptor_t udp_descriptor = {
    .source = 68,
    .destination = 67
  };
  DPAUCS_address_pair_t ap = {
    .destination = DPAUCS_ADDR_IPv4((0xFF,0xFF,0xFF,0xFF,0xFF,0xFF),(255,255,255,255)),
    .source      = DPAUCS_ADDR_IPv4((0),(0))
  };
  memcpy( ap.source->mac, interface->mac, sizeof(DPAUCS_mac_t) );
  DPAUCS_addressPairToMixed( &udp_descriptor.fromTo, &ap );
  DPA_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
  DPAUCS_dhcp_t dhcp;
  memset( &dhcp, 0, sizeof(dhcp) );
  dhcp.op = DHCP_OP_BOOT_REQUEST,
  dhcp.htype = 1,
  dhcp.hlen = sizeof(DPAUCS_mac_t),
  dhcp.xid = lease->lease_start,
  dhcp.flags = DPA_htob16((uint16_t)1<<DHCP_FLAG_BROADCAST),
  lease->state = DHCP_STATE_REQUEST_SENT;
  memcpy( dhcp.siaddr, sip, 4 );
  memcpy( dhcp.chaddr, interface->mac, sizeof(DPAUCS_mac_t) );
  DPA_stream_fillWrite( stream, 0, 192 );
  const uint8_t dhcp_options[] = {
    DHCP_OPTION_MAGIC,
    DHCP_OPTION( MESSAGE_TYPE, DHCP_MESSAGE_TYPE_DHCPREQUEST ),
    DHCP_OPTION( REQUEST_IP_ADDRESS, rip[0], rip[1], rip[2], rip[3] ),
    DHCP_OPTION( SERVER_IDENTIFIER, sip[0], sip[1], sip[2], sip[3] ),
    DHCP_OPTION( PARAMETER_REQUEST_LIST,
      DHCP_OPTION_SUBNET_MASK,
      DHCP_OPTION_ROUTER,
      DHCP_OPTION_DOMAIN_NAME_SERVER,
      DHCP_OPTION_DOMAIN_NAME,
      DHCP_OPTION_DOMAIN_SEARCH,
      DHCP_OPTION_CLASSLESS_STATIC_ROUTE
    ),
    DHCP_OPTION_HOST_NAME
  };
  DPA_stream_referenceWrite( stream, dhcp_options, sizeof(dhcp_options) );
  const char* hostname_end = DPAUCS_hostname;
  for( ; *hostname_end; hostname_end++ );
  size_t hostname_length = hostname_end - DPAUCS_hostname;
  if( hostname_length > 0xFF )
    hostname_length = 0xFF;
  DPA_stream_referenceWrite( stream, (uint8_t[]){hostname_length}, 1 );
  DPA_stream_referenceWrite( stream, DPAUCS_hostname, hostname_length );
  DPA_stream_referenceWrite( stream, (uint8_t[]){DHCP_OPTION_END}, 1 );
  DPAUCS_udp_transmit( &udp_descriptor, (linked_data_list_t[]){{
    .size = sizeof(dhcp),
    .data = &dhcp,
    .next = 0
  }}, stream );
  DPAUCS_layer3_destroyTransmissionStream( stream );
}

static void dhcp_discover( const DPAUCS_interface_t* interface ){
  if( !interface->link_up )
    return;
  DPAUCS_udp_descriptor_t udp_descriptor = {
    .source = 68,
    .destination = 67
  };
  DPAUCS_address_pair_t ap = {
    .destination = DPAUCS_ADDR_IPv4((0xFF,0xFF,0xFF,0xFF,0xFF,0xFF),(255,255,255,255)),
    .source      = DPAUCS_ADDR_IPv4((0),(0))
  };
  dhcp_remove_lease( interface );
  dhcp_lease_t* lease = dhcp_alloc_lease();
  if(!lease){
    DPA_LOG("Failed to allocate a dhcp lease\n");
    return;
  }
  lease->state = DHCP_STATE_DISCOVER_SENT;
  lease->interface = interface;
  memcpy( ap.source->mac, interface->mac, sizeof(DPAUCS_mac_t) );
  DPAUCS_addressPairToMixed( &udp_descriptor.fromTo, &ap );
  DPA_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
  DPAUCS_dhcp_t dhcp;
  memset( &dhcp, 0, sizeof(dhcp) );
  dhcp.op = DHCP_OP_BOOT_REQUEST,
  dhcp.htype = 1,
  dhcp.hlen = sizeof(DPAUCS_mac_t),
  dhcp.xid = lease->lease_start,
  dhcp.flags = DPA_htob16((uint16_t)1<<DHCP_FLAG_BROADCAST),
  memcpy( dhcp.chaddr, interface->mac, sizeof(DPAUCS_mac_t) );
  DPA_stream_fillWrite( stream, 0, 192 );
  static const flash uint8_t dhcp_options[] = {
    DHCP_OPTION_MAGIC,
    DHCP_OPTION( MESSAGE_TYPE, DHCP_MESSAGE_TYPE_DHCPDISCOVER ),
    DHCP_OPTION_END
  };
  DPA_stream_progmemWrite( stream, dhcp_options, sizeof(dhcp_options) );
  DPAUCS_udp_transmit( &udp_descriptor, (linked_data_list_t[]){{
    .size = sizeof(dhcp),
    .data = &dhcp,
    .next = 0
  }}, stream );
  DPAUCS_layer3_destroyTransmissionStream( stream );
}

static void dhcp_release( DPAUCS_interface_t* interface ){
  if( !interface->link_up ){
    dhcp_remove_lease( interface );
    return;
  }
  dhcp_lease_t* lease = dhcp_get_lease(interface);
  if( !lease )
    return;
  if( lease->state == DHCP_STATE_ACCEPTED ){
    DPAUCS_udp_descriptor_t udp_descriptor = {
      .source = 68,
      .destination = 67
    };
    DPAUCS_address_pair_t ap = {
      .destination = DPAUCS_ADDR_IPv4((0xFF,0xFF,0xFF,0xFF,0xFF,0xFF),(255,255,255,255)),
      .source      = DPAUCS_ADDR_IPv4((0),(0))
    };
    memcpy( ap.source->mac, interface->mac, sizeof(DPAUCS_mac_t) );
    DPAUCS_addressPairToMixed( &udp_descriptor.fromTo, &ap );
    DPA_stream_t* stream = DPAUCS_layer3_createTransmissionStream();
    DPAUCS_dhcp_t dhcp;
    memset( &dhcp, 0, sizeof(dhcp) );
    dhcp.op = DHCP_OP_BOOT_REQUEST,
    dhcp.htype = 1,
    dhcp.hlen = sizeof(DPAUCS_mac_t),
    dhcp.xid = lease->lease_start,
    dhcp.flags = DPA_htob16((uint16_t)1<<DHCP_FLAG_BROADCAST),
    memcpy( dhcp.chaddr, interface->mac, sizeof(DPAUCS_mac_t) );
    DPA_stream_fillWrite( stream, 0, 192 );
    static const flash uint8_t dhcp_options[] = {
      DHCP_OPTION_MAGIC,
      DHCP_OPTION( MESSAGE_TYPE, DHCP_MESSAGE_TYPE_DHCPRELEASE ),
      DHCP_OPTION_END
    };
    DPA_stream_progmemWrite( stream, dhcp_options, sizeof(dhcp_options) );
    DPAUCS_udp_transmit( &udp_descriptor, (linked_data_list_t[]){{
      .size = sizeof(dhcp),
      .data = &dhcp,
      .next = 0
    }}, stream );
    DPAUCS_layer3_destroyTransmissionStream( stream );
  }
  dhcp_remove_lease( interface );
}

void DPAUCS_dhcp_client_update_interface( DPAUCS_interface_t* interface ){
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
    DPAUCS_dhcp_client_update_interface( interface );
}

DPAUCS_INIT {
  if( !DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 68, &dhcp_client_service ) ){
    static const flash char message[] = {"Failed to add DHCP service!\n"};
    DPAUCS_fatal(message);
  }
}

DPAUCS_SHUTDOWN {
  DPAUCS_remove_service( DPAUCS_ANY_ADDRESS, 68 );
}

DPA_LOOSE_LIST_ADD( DPAUCS_event_list, &interface_event_handler )

const flash DPAUCS_service_t dhcp_client_service = {
  .tos = PROTOCOL_UDP,
  .onreceive = &onreceive
};
