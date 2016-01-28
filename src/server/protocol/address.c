#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/anyAddress.h>

bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t* address){
  switch( address->type ){
#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: {
      return !~((const DPAUCS_logicAddress_IPv4_t*)address)->address;
    } break;
#endif
    case DPAUCS_AT_UNKNOWN: break;
  }
  return false;
}

bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t* a,const DPAUCS_logicAddress_t* b){
  if( a->type != b->type )
    return false;
  switch(a->type){
#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: {
      return ((const DPAUCS_logicAddress_IPv4_t*)a)->address == ((const DPAUCS_logicAddress_IPv4_t*)b)->address;
    } break;
#endif
    case DPAUCS_AT_UNKNOWN: break;
  }
  return false;
}

bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t* address){
  return !DPAUCS_isBroadcast(address);
}

bool DPAUCS_copy_logicAddress( DPAUCS_logicAddress_t* dst, const DPAUCS_logicAddress_t* src ){
  if( dst->type != src->type )
    return false;
  switch(dst->type){
#ifdef USE_IPv4
    case DPAUCS_AT_IPv4: {
      *(DPAUCS_logicAddress_IPv4_t*)dst = *(const DPAUCS_logicAddress_IPv4_t*)src;
      return true;
    } break;
#endif
    case DPAUCS_AT_UNKNOWN: break;
  }
  return false;
}

bool DPAUCS_persistMixedAddress( DPAUCS_mixedAddress_pair_t* map ){
  if( map->flags & DPAUCS_F_AP_PERSISTENT
   || map->type != DPAUCS_AP_ADDRESS
  ) return false;
  const DPAUCS_logicAddress_t* src = DPAUCS_get_logicAddress( &map->address.source->logicAddress );
  if( src ){
    map->flags |= DPAUCS_F_AP_SRC_SVR_OR_APC;
    map->logicAddress.source = src;
  }else{
    map->logicAddress.source = &DPAUCS_arpCache_register( map->address.source )->logicAddress;
  }
  const DPAUCS_logicAddress_t* dest = DPAUCS_get_logicAddress( &map->address.source->logicAddress );
  if( dest ){
    map->flags |= DPAUCS_F_AP_DST_SVR_OR_APC;
    map->logicAddress.destination = dest;
  }else{
    map->logicAddress.destination = &DPAUCS_arpCache_register( map->address.destination )->logicAddress;
  }
  return true;
}

bool DPAUCS_freeMixedAddress( DPAUCS_mixedAddress_pair_t* map ){
  if(!( map->flags & DPAUCS_F_AP_PERSISTENT )
     || map->type != DPAUCS_AP_LOGIC_ADDRESS
  ) return false;
  if(!( map->flags & DPAUCS_F_AP_SRC_SVR_OR_APC ))
    DPAUCS_arpCache_deregister( map->logicAddress.source );
  if(!( map->flags & DPAUCS_F_AP_DST_SVR_OR_APC ))
    DPAUCS_arpCache_deregister( map->logicAddress.destination );
  return true;
}

bool DPAUCS_addressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_address_pair_t* address ){
  mixed->flags = 0;
  mixed->type = DPAUCS_AP_ADDRESS;
  mixed->address = *address;
  return true;
}

bool DPAUCS_logicAddressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_logicAddress_pair_t* address ){
  mixed->flags = 0;
  mixed->type = DPAUCS_AP_LOGIC_ADDRESS;
  mixed->logicAddress = *address;
  return true;
}

const DPAUCS_logicAddress_t* DPAUCS_mixedPairComponentToLogicAddress(
  const DPAUCS_mixedAddress_pair_t* mixed,
  bool source_or_destination
){
  switch( mixed->type ){
    case DPAUCS_AP_ADDRESS:
      return source_or_destination
       ? &mixed->address.source->logicAddress
       : &mixed->address.destination->logicAddress;
    case DPAUCS_AP_LOGIC_ADDRESS:
      return source_or_destination
       ? mixed->logicAddress.source
       : mixed->logicAddress.destination;
  }
  return 0;
}

const DPAUCS_address_t* DPAUCS_mixedPairComponentToAddress(
  const DPAUCS_mixedAddress_pair_t* mixed,
  bool source_or_destination
){
  switch( mixed->type ){
    case DPAUCS_AP_ADDRESS:
      return source_or_destination
       ? mixed->address.source
       : mixed->address.destination;
    case DPAUCS_AP_LOGIC_ADDRESS: {
      if(!( mixed->flags & DPAUCS_F_AP_PERSISTENT ))
        break;
      if(!( mixed->flags & ( source_or_destination
                              ? DPAUCS_F_AP_SRC_SVR_OR_APC
                              : DPAUCS_F_AP_DST_SVR_OR_APC
      ))) break;
      const DPAUCS_logicAddress_t* logicAddress = source_or_destination
                                            ? mixed->logicAddress.source
                                            : mixed->logicAddress.destination;
      return DPAUCS_arpCache_getAddress( logicAddress );
    }
  }
  return 0;
}

bool DPAUCS_mixedPairToAddress( DPAUCS_address_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed ){
  switch( mixed->type ){
    case DPAUCS_AP_ADDRESS: {
      address->source = mixed->address.source;
      address->destination = mixed->address.destination;
    } return true;
    case DPAUCS_AP_LOGIC_ADDRESS: break;
  }
  return false;
}

bool DPAUCS_mixedPairToLogicAddress( DPAUCS_logicAddress_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed ){
  switch( mixed->type ){
    case DPAUCS_AP_ADDRESS: {
      address->source = &mixed->address.source->logicAddress;
      address->destination = &mixed->address.destination->logicAddress;
    } return true;
    case DPAUCS_AP_LOGIC_ADDRESS: {
      *address = mixed->logicAddress;
    } return true;
  }
  return false;
}

bool DPAUCS_mixedPairEqual( const DPAUCS_mixedAddress_pair_t* ma, const DPAUCS_mixedAddress_pair_t* mb ){
  DPAUCS_logicAddress_pair_t la,lb;
  return DPAUCS_mixedPairToLogicAddress( &la, ma )
      && DPAUCS_mixedPairToLogicAddress( &lb, mb )
      && DPAUCS_compare_logicAddress( la.source, lb.source )
      && DPAUCS_compare_logicAddress( la.destination, lb.destination );
}

enum DPAUCS_address_types DPAUCS_mixedPairGetType( const DPAUCS_mixedAddress_pair_t* addr ){
  switch( addr->type ){
    case DPAUCS_AP_ADDRESS      : return addr->address.source->type;
    case DPAUCS_AP_LOGIC_ADDRESS: return addr->logicAddress.source->type;
  }
  return DPAUCS_AT_UNKNOWN;
}
