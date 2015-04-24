#include <string.h>
#include <protocol/ip_stack.h>

// Should be initialized with zeros
static DPAUCS_ipPacketInfo_t incompleteIpPackageInfos[DPAUCS_MAX_INCOMPLETE_IP_PACKETS];

unsigned incompleteIpPackageInfoOffset = 0;
static DPAUCS_ipPacketInfo_t* getNextIncompleteIpPacket(){
  DPAUCS_ipPacketInfo_t* next = incompleteIpPackageInfos + incompleteIpPackageInfoOffset;
  incompleteIpPackageInfoOffset = ( incompleteIpPackageInfoOffset + 1) % DPAUCS_MAX_INCOMPLETE_IP_PACKETS;
  next->valid = false;
  return next;
}

DPAUCS_ip_fragment_t** DPAUCS_allocIpFragment( DPAUCS_ipPacketInfo_t* packet, uint16_t size ){
  DPAUCS_ipPacketInfo_t* info = 0;
  if(!packet){
    info = getNextIncompleteIpPacket();
  }else if(
       packet < incompleteIpPackageInfos
    || packet > incompleteIpPackageInfos + DPAUCS_MAX_INCOMPLETE_IP_PACKETS
  ){
    for(unsigned i=0;i<DPAUCS_MAX_INCOMPLETE_IP_PACKETS;i++)
      if( DPAUCS_areFragmentsFromSameIpPacket( incompleteIpPackageInfos+i, packet ) ){
	info = incompleteIpPackageInfos+i;
	break;
      }
    if(!info){
      info = getNextIncompleteIpPacket();
      memcpy(info,packet,sizeof(*info));
    }
  }else{
    info = packet;
  }
  return DPAUCS_allocFragmentType(DPAUCS_ip_fragment_t,size);
}

bool DPAUCS_isNextIpFragment( DPAUCS_ip_fragment_t* ipf ){
  return !ipf->offset || ipf->info->offset == ipf->offset;
}

struct searchFollowingIpFragmentArgs {
  DPAUCS_ip_fragment_t* prev;
  DPAUCS_ip_fragment_t** result;
};

static bool searchFollowingIpFragment( DPAUCS_fragment_t** f, void* arg ){
  struct searchFollowingIpFragmentArgs* args = arg;
  DPAUCS_ip_fragment_t** ipf_ptr = (DPAUCS_ip_fragment_t**)f;
  DPAUCS_ip_fragment_t* ipf = *ipf_ptr;
  if( ipf->info != args->prev->info || !DPAUCS_isNextIpFragment(ipf) )
    return true;
  args->result = ipf_ptr;
  return false;
}

bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ipPacketInfo_t* a, DPAUCS_ipPacketInfo_t* b ){
  return (
   a == b || 
    ( a->id == b->id
   && a->tos == b->tos
   && a->destIp == b->destIp
   && a->srcIp == b->srcIp
   && !memcmp(a->srcMac,b->srcMac,6)
   && !memcmp(a->destMac,b->destMac,6)
  ));
}

DPAUCS_ip_fragment_t** DPAUCS_searchFollowingIpFragment( DPAUCS_ip_fragment_t* ipf ){
  struct searchFollowingIpFragmentArgs args = {
    ipf,
    0
  };
  DPAUCS_eachFragmentByType( DPAUCS_ip_fragment_t, &searchFollowingIpFragment, &args );
  return args.result;
}

void DPAUCS_updateIpPackatOffset( DPAUCS_ip_fragment_t* f ){
  if(!DPAUCS_isNextIpFragment(f))
    return;
  f->info->offset += f->length;
}

static bool removeIpFragment( DPAUCS_fragment_t** f, void* packet_ptr ){
  if( ((DPAUCS_ip_fragment_t*)*f)->info == packet_ptr )
    DPAUCS_removeFragment(f);
  return true;
}

void DPAUCS_removeIpPacket( DPAUCS_ipPacketInfo_t* ipf ){
  if(!ipf->valid)
    return;
  ipf->valid = false;
  DPAUCS_eachFragmentByType( DPAUCS_ip_fragment_t, &removeIpFragment, ipf );
}

static void ipFragmentDestructor(DPAUCS_fragment_t** f){
  DPAUCS_removeIpPacket(((DPAUCS_ip_fragment_t*)*f)->info);
}

extern DPAUCS_fragment_info_t DPAUCS_ip_fragment_info;
DPAUCS_fragment_info_t DPAUCS_ip_fragment_info = {
  &ipFragmentDestructor
};
