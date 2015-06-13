#include <string.h>
#include <protocol/ip_stack.h>

// Should be initialized with zeros
static DPAUCS_layer3_packetInfo_t incompletePackageInfos[DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS];

unsigned DPAUCS_layer3_getPacketTypeSize(enum DPAUCS_fragmentType type){
  switch(type){
    case DPAUCS_FRAGMENT_TYPE_IPv4: return sizeof(DPAUCS_IPv4_packetInfo_t);
  }
  return 0;
}

unsigned DPAUCS_layer3_getFragmentTypeSize(enum DPAUCS_fragmentType type){
  switch(type){
    case DPAUCS_FRAGMENT_TYPE_IPv4: return sizeof(DPAUCS_IPv4_fragment_t);
  }
  return 0;
}

bool DPAUCS_areFragmentsFromSameIPv4Packet( DPAUCS_IPv4_packetInfo_t* a, DPAUCS_IPv4_packetInfo_t* b ){
  return (
      a->id == b->id
   && a->tos == b->tos
   && a->src.ip == b->src.ip
   && a->src.ip == b->src.ip
   && !memcmp(a->src.address.mac,b->src.address.mac,6)
   && !memcmp(a->dest.address.mac,b->dest.address.mac,6)
  );
}

bool DPAUCS_areFragmentsFromSameIpPacket( DPAUCS_ip_packetInfo_t* a, DPAUCS_ip_packetInfo_t* b ){
  if( a == b )
    return true;
  if( a->type != b->type )
    return false;
  if(  (DPAUCS_layer3_packetInfo_t*)a < incompletePackageInfos
    || (DPAUCS_layer3_packetInfo_t*)a >= incompletePackageInfos + DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS
    || (DPAUCS_layer3_packetInfo_t*)b < incompletePackageInfos
    || (DPAUCS_layer3_packetInfo_t*)b >= incompletePackageInfos + DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS
  ){
    switch(a->type){
      case DPAUCS_FRAGMENT_TYPE_IPv4: return DPAUCS_areFragmentsFromSameIPv4Packet(
        (DPAUCS_IPv4_packetInfo_t*)a,
        (DPAUCS_IPv4_packetInfo_t*)b
      ); break;
    }
  }
  return false;
}

unsigned incompleteIpPackageInfoOffset = 0;
static DPAUCS_ip_packetInfo_t* getNextIncompleteIpPacket(){
  DPAUCS_ip_packetInfo_t* next = &incompletePackageInfos[incompleteIpPackageInfoOffset].ip_packetInfo;
  incompleteIpPackageInfoOffset = ( incompleteIpPackageInfoOffset + 1) % DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS;
  next->valid = false;
  return next;
}

DPAUCS_ip_fragment_t** DPAUCS_layer3_allocFragment( DPAUCS_ip_packetInfo_t* packet, uint16_t size ){
  DPAUCS_ip_packetInfo_t* info = 0;
  if(!packet){
    info = getNextIncompleteIpPacket();
  }else if(
       (DPAUCS_layer3_packetInfo_t*)packet < incompletePackageInfos
    || (DPAUCS_layer3_packetInfo_t*)packet >= incompletePackageInfos + DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS
  ){
    info = DPAUCS_normalize_ip_packet_info_ptr( packet );
    if(!info)
      info = DPAUCS_save_ip_packet_info(info);
  }else{
    info = packet;
  }
  DPAUCS_ip_fragment_t** f_ptr = (DPAUCS_ip_fragment_t**)DPAUCS_createFragment(packet->type,size+DPAUCS_layer3_getFragmentTypeSize(packet->type));
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

DPAUCS_ip_packetInfo_t* DPAUCS_save_ip_packet_info( DPAUCS_ip_packetInfo_t* packet ){
  DPAUCS_ip_packetInfo_t* info = getNextIncompleteIpPacket();
  memcpy(info,packet,DPAUCS_layer3_getPacketTypeSize(packet->type));
  return info;
}

DPAUCS_ip_packetInfo_t* DPAUCS_normalize_ip_packet_info_ptr(DPAUCS_ip_packetInfo_t* ipf){
  for(unsigned i=0;i<DPAUCS_MAX_INCOMPLETE_LAYER3_PACKETS;i++)
    if( incompletePackageInfos[i].ip_packetInfo.valid && DPAUCS_areFragmentsFromSameIpPacket( &incompletePackageInfos[i].ip_packetInfo, ipf ) )
      return &incompletePackageInfos[i].ip_packetInfo;
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

DPAUCS_ip_fragment_t** DPAUCS_searchFollowingIpFragment( DPAUCS_ip_fragment_t* ipf ){
  struct searchFollowingIpFragmentArgs args = {
    ipf,
    0
  };
  DPAUCS_eachFragment( ipf->fragmentInfo.type, &searchFollowingIpFragment, &args );
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

void DPAUCS_removeIpPacket( DPAUCS_ip_packetInfo_t* ipf ){
  if(!ipf->valid)
    return;
  ipf->valid = false;
  DPAUCS_eachFragment( ipf->type, &removeIpFragment, ipf );
  if(ipf->onremove)
    (ipf->onremove)(ipf);
}

void DPAUCS_removeIpFragment( DPAUCS_ip_fragment_t** f ){
  removeIpFragment((DPAUCS_fragment_t**)f,(*f)->info);
}

extern const DPAUCS_fragment_info_t DPAUCS_ip_fragment_info;
const DPAUCS_fragment_info_t DPAUCS_ip_fragment_info = {
  &ipFragmentDestructor
};
