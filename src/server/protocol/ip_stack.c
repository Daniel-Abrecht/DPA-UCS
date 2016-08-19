#include <string.h>
#include <DPA/UCS/protocol/IPv4.h>
#include <DPA/UCS/protocol/ip_stack.h>

// Should be initialized with zeros
static char incompletePackageInfos[DPA_MAX_PACKET_INFO_BUFFER_SIZE];
DPAUCS_ip_packetInfo_t* incompletePackageInfos_end;

bool DPAUCS_layer3_areFragmentsFromSamePacket( DPAUCS_ip_packetInfo_t* a, DPAUCS_ip_packetInfo_t* b ){
  if( a == b )
    return true;
  if( a->handler != b->handler )
    return false;
  if( (char*)a < incompletePackageInfos || a >= incompletePackageInfos_end
   || (char*)b < incompletePackageInfos || b >= incompletePackageInfos_end
  ){
    if( a->handler->isSamePacket )
      return a->handler->isSamePacket(a,b);
  }
  return false;
}

static inline size_t getRealPacketInfoSize( void ){
  static size_t size = 0;
  if( !size ){
    DPAUCS_EACH_FRAGMENT_HANDLER( it ){
      size_t s = (*it)->packetInfo_size;
      if( size < s )
        size = s;
    }
  }
  return size;
}

static inline void nextPacketInfo( DPAUCS_ip_packetInfo_t** it  ){
  *it += getRealPacketInfoSize();
}

static inline DPAUCS_ip_packetInfo_t* getNextIncompletePacket( void ){
  static DPAUCS_ip_packetInfo_t* it = (DPAUCS_ip_packetInfo_t*)incompletePackageInfos;
  it->valid = false;
  nextPacketInfo( &it );
  if( it > incompletePackageInfos_end )
    it = (DPAUCS_ip_packetInfo_t*)incompletePackageInfos;
  return it;
}

DPAUCS_ip_fragment_t** DPAUCS_layer3_allocFragment( DPAUCS_ip_packetInfo_t* packet, uint16_t size ){
  DPAUCS_ip_packetInfo_t* info = 0;
  if(!packet){
    info = getNextIncompletePacket();
  }else if( (char*)packet < incompletePackageInfos || packet >= incompletePackageInfos_end ){
    info = DPAUCS_layer3_normalize_packet_info_ptr( packet );
    if(!info)
      info = DPAUCS_layer3_save_packet_info(info);
  }else{
    info = packet;
  }
  DPAUCS_ip_fragment_t** f_ptr = (DPAUCS_ip_fragment_t**)DPAUCS_createFragment(packet->handler,size);
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

DPAUCS_ip_packetInfo_t* DPAUCS_layer3_save_packet_info( DPAUCS_ip_packetInfo_t* packet ){
  DPAUCS_ip_packetInfo_t* info = getNextIncompletePacket();
  memcpy(info,packet,packet->handler->packetInfo_size);
  return info;
}

DPAUCS_ip_packetInfo_t* DPAUCS_layer3_normalize_packet_info_ptr(DPAUCS_ip_packetInfo_t* ipf){
  for(
    DPAUCS_ip_packetInfo_t* it = (DPAUCS_ip_packetInfo_t*)incompletePackageInfos;
    it < incompletePackageInfos_end;
    nextPacketInfo( &it )
  ) if( it->valid && DPAUCS_layer3_areFragmentsFromSamePacket( it, ipf ) )
      return it;
  return 0;
}

bool DPAUCS_layer3_isNextFragment( DPAUCS_ip_fragment_t* ipf ){
  return !ipf->offset || ipf->info->offset == ipf->offset;
}

struct searchFollowingFragmentArgs {
  DPAUCS_ip_fragment_t* prev;
  DPAUCS_ip_fragment_t** result;
};

static bool searchFollowingFragment( DPAUCS_fragment_t** f, void* arg ){
  struct searchFollowingFragmentArgs* args = arg;
  DPAUCS_ip_fragment_t** ipf_ptr = (DPAUCS_ip_fragment_t**)f;
  DPAUCS_ip_fragment_t* ipf = *ipf_ptr;
  bool isNext = DPAUCS_layer3_isNextFragment(ipf);
  if( !isNext || ipf->info != args->prev->info )
    return true;
  args->result = ipf_ptr;
  return false;
}

DPAUCS_ip_fragment_t** DPAUCS_layer3_searchFollowingFragment( DPAUCS_ip_fragment_t* ipf ){
  struct searchFollowingFragmentArgs args = {
    ipf,
    0
  };
  DPAUCS_eachFragment( ipf->info->handler, &searchFollowingFragment, &args );
  return args.result;
}

void DPAUCS_layer3_updatePackatOffset( DPAUCS_ip_fragment_t* f ){
  if(!DPAUCS_layer3_isNextFragment(f))
    return;
  f->info->offset += f->length;
}

static bool removeIpFragment( DPAUCS_fragment_t** f, void* packet_ptr ){
  if( ((DPAUCS_ip_fragment_t*)*f)->info == packet_ptr )
    DPAUCS_removeFragment(f);
  return true;
}

void DPAUCS_layer3_removePacket( DPAUCS_ip_packetInfo_t* ipf ){
  if(!ipf->valid)
    return;
  ipf->valid = false;
  DPAUCS_eachFragment( ipf->handler, &removeIpFragment, ipf );
  if(ipf->onremove)
    (ipf->onremove)(ipf);
}

void DPAUCS_layer3_removeFragment( DPAUCS_ip_fragment_t** f ){
  removeIpFragment((DPAUCS_fragment_t**)f,(*f)->info);
}
