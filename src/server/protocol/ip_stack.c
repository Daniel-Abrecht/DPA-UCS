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
    || packet >= incompleteIpPackageInfos + DPAUCS_MAX_INCOMPLETE_IP_PACKETS
  ){
    info = DPAUCS_normalize_ip_packet_info_ptr( packet );
    if(!info)
      info = DPAUCS_save_ip_packet_info(info);
  }else{
    info = packet;
  }
  DPAUCS_ip_fragment_t** f_ptr = DPAUCS_allocFragmentType(DPAUCS_ip_fragment_t,size);
  if(!f_ptr)
    return 0;
  DPAUCS_ip_fragment_t* f = *f_ptr;
  if(!info->valid){ // if packet info and all its fragments were removed to gain enought space for new fragment
    DPAUCS_removeFragment((DPAUCS_fragment_t**)f_ptr);
    return 0;
  }
  f->info = info;
  return f_ptr;
}

DPAUCS_ipPacketInfo_t* DPAUCS_save_ip_packet_info(DPAUCS_ipPacketInfo_t* packet){
  DPAUCS_ipPacketInfo_t* info = getNextIncompleteIpPacket();
  memcpy(info,packet,sizeof(*info));
  return info;
}

DPAUCS_ipPacketInfo_t* DPAUCS_normalize_ip_packet_info_ptr(DPAUCS_ipPacketInfo_t* ipf){
  for(unsigned i=0;i<DPAUCS_MAX_INCOMPLETE_IP_PACKETS;i++)
    if( DPAUCS_areFragmentsFromSameIpPacket( incompleteIpPackageInfos+i, ipf ) )
      return incompleteIpPackageInfos + i;
  return 0;
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
  bool isNext = DPAUCS_isNextIpFragment(ipf);
  if( !isNext || ipf->info != args->prev->info )
    return true;
  args->result = ipf_ptr;
  return false;
}

bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ipPacketInfo_t* a, DPAUCS_ipPacketInfo_t* b ){
  if( a == b )
    return true;
  if(  a < incompleteIpPackageInfos
    || a >= incompleteIpPackageInfos + DPAUCS_MAX_INCOMPLETE_IP_PACKETS
    || b < incompleteIpPackageInfos
    || b >= incompleteIpPackageInfos + DPAUCS_MAX_INCOMPLETE_IP_PACKETS
  ){
    return (
        a->id == b->id
     && a->tos == b->tos
     && a->destIp == b->destIp
     && a->srcIp == b->srcIp
     && !memcmp(a->srcMac,b->srcMac,6)
     && !memcmp(a->destMac,b->destMac,6)
    );
  }
  return false;
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

static bool removePacketToo = true;

static bool removeIpFragment( DPAUCS_fragment_t** f, void* packet_ptr ){
  if( ((DPAUCS_ip_fragment_t*)*f)->info == packet_ptr ){
    removePacketToo = false;
    DPAUCS_removeFragment(f);
    removePacketToo = true;
  }
  return true;
}

static void ipFragmentDestructor(DPAUCS_fragment_t** f){
  if(removePacketToo)
    DPAUCS_removeIpPacket(((DPAUCS_ip_fragment_t*)*f)->info);
}

void DPAUCS_removeIpPacket( DPAUCS_ipPacketInfo_t* ipf ){
  if(!ipf->valid)
    return;
  ipf->valid = false;
  DPAUCS_eachFragmentByType( DPAUCS_ip_fragment_t, &removeIpFragment, ipf );
}

void DPAUCS_removeIpFragment( DPAUCS_ip_fragment_t** f ){
  removeIpFragment((DPAUCS_fragment_t**)f,(*f)->info);
}

extern const DPAUCS_fragment_info_t DPAUCS_ip_fragment_info;
const DPAUCS_fragment_info_t DPAUCS_ip_fragment_info = {
  &ipFragmentDestructor
};
