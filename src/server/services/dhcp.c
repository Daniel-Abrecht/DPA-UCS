#include <DPA/UCS/server.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/protocol/udp.h>
#include <DPA/UCS/services/dhcp.h>

DPA_MODULE( dhcp ){
  DPA_DEPENDENCY( udp );
}

static void onreceive( void* request, void* data, size_t size ){
  DPAUCS_udp_descriptor_t reply;
  DPAUCS_udp_make_reply_descriptor(request,&reply);
  (void)data;
  (void)size;
  DPA_LOG("DHCP packet\n");
}

const flash DPAUCS_service_t dhcp_service = {
  .tos = PROTOCOL_UDP,
  .onreceive = &onreceive
};

DPAUCS_INIT {
  if( !DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 68, &dhcp_service ) ){
    static const flash char message[] = {"Failed to add DHCP service!\n"};
    DPAUCS_fatal(message);
  }
}

DPAUCS_SHUTDOWN {
  DPAUCS_remove_service( DPAUCS_ANY_ADDRESS, 68 );
}

