#include <stdio.h>
#include <string.h>
#include <server.h>
#include <binaryUtils.h>
#include <protocol/ip.h>
#include <protocol/ip_stack.h>

static uint16_t MTU = 1500;

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

void hexdump(const unsigned char* x,unsigned s,unsigned y){
  unsigned i;
  for(i=0;i<s;i++,x++)
    printf("%.2x%c",(int)*x,((i+1)%y)?' ':'\n');
  if((i%y)) printf("\n");
}

static void IPv4_sendFromTo(
  uint32_t sip,
  uint32_t dip,
  uint8_t* smac,
  uint8_t* dmac,
  void* payload, 
  uint16_t length
){
  printf(
    "IPv4 send src: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
    " | dest: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
    "ip src: %u.%u.%u.%u | "
    "ip dest: %u.%u.%u.%u\n"
    "length: %u | data:\n",

    smac[0], smac[1],
    smac[2], smac[3],
    smac[4], smac[5],

    dmac[0], dmac[1],
    dmac[2], dmac[3],
    dmac[4], dmac[5],

    ( sip >> 24 ) & 0xFF,
    ( sip >> 16 ) & 0xFF,
    ( sip >>  8 ) & 0xFF,
    ( sip >>  0 ) & 0xFF,

    ( dip >> 24 ) & 0xFF,
    ( dip >> 16 ) & 0xFF,
    ( dip >>  8 ) & 0xFF,
    ( dip >>  0 ) & 0xFF,

    (unsigned)length

  );
  hexdump(payload,length,16);
}

static void IPv4_sendBackHandler(void* x, void* payload, uint16_t length){
  DPAUCS_ipPacketInfo_t* info = x;
  IPv4_sendFromTo(
    info->destIp,
    info->srcIp,
    info->destMac,
    info->srcMac,
    payload,
    length
  );
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

  DPAUCS_ip_fragment_t fragment;
  DPAUCS_ipPacketInfo_t ipInfo;
  ipInfo.srcIp = source;
  ipInfo.destIp = destination;
  memcpy(ipInfo.srcMac,info->source_mac,6);
  memcpy(ipInfo.destMac,info->destination_mac,6);
  ipInfo.id = ip->id;
  ipInfo.tos = ip->tos;
  ipInfo.offset = 0;
  ipInfo.valid = true;
  
  uint16_t headerlength = (ip->version_ihl & 0x0F) * 4;
  
  fragment.offset = (uint16_t)( (uint16_t)( ip->flags_offset1 & 0x1F ) | (uint16_t)ip->offset2 << 5u ) * 8u;
  fragment.length = btoh16(ip->length) - headerlength;
  fragment.flags = ( ip->flags_offset1 >> 5 ) & 0x07;
  
  uint8_t* payload = ((uint8_t*)ip) + headerlength;
  
  if(fragment.length>=MTU)
    return;
  
  DPAUCS_ipPacketInfo_t* ipi = DPAUCS_normalize_ip_packet_info_ptr(&ipInfo);
  bool isNext;
  if(ipi){
    fragment.info = ipi;
    isNext = DPAUCS_isNextIpFragment(&fragment);
  }else{
    isNext = !fragment.offset; 
  }

  if( fragment.flags & IPv4_FLAG_MORE_FRAGMENTS || !isNext ){
    if(!ipi)
      ipi = DPAUCS_save_ip_packet_info(&ipInfo);
    fragment.info = ipi;
    if(isNext){
      DPAUCS_updateIpPackatOffset(&fragment);
      DPAUCS_ip_fragment_t* f = &fragment;
      DPAUCS_ip_fragment_t** f_ptr = &f;
      while(true){
        if(
            !(*handler)(f->info,&IPv4_sendBackHandler,f->offset,f->length,payload,!(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS))
         || !(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS)
        ){
          DPAUCS_removeIpPacket(f->info);
        }else{
	  DPAUCS_removeIpFragment(f_ptr);
	}
        f_ptr = DPAUCS_searchFollowingIpFragment(*f_ptr);
        if(!f_ptr)
          break;
        f = *f_ptr;
	payload = (uint8_t*)(f+1);
      }
    }else{
      DPAUCS_ip_fragment_t** f_ptr = DPAUCS_allocIpFragment(ipi,fragment.length);
      if(!f_ptr){
	printf("DPAUCS_allocIpFragment failed!\n");
        return;
      }
      memcpy(
        ((uint8_t*)*f_ptr) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        ((uint8_t*)&fragment) + DPAUCS_IP_FRAGMENT_DATA_OFFSET,
        sizeof(DPAUCS_ip_fragment_t) - DPAUCS_IP_FRAGMENT_DATA_OFFSET
      );
      memcpy((*f_ptr)+1,payload,fragment.length);
      return;
    }
  }else{
    (*handler)(ipi?ipi:&ipInfo,&IPv4_sendBackHandler,fragment.offset,fragment.length,payload,!(fragment.flags & IPv4_FLAG_MORE_FRAGMENTS));
    if(ipi)
      DPAUCS_removeIpPacket(ipi);
    ipi = 0;
  }

}

void DPAUCS_ip_handler( DPAUCS_packet_info* info ){
  DPAUCS_ip_t* ip = info->payload;
  switch( ip->version >> 4 ){
    case 4: IPv4_handler(info,&ip->ipv4); break;
  }
}

