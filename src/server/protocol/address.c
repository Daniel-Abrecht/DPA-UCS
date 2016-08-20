#include <DPA/UCS/server.h>
#include <DPA/UCS/protocol/arp.h>
#include <DPA/UCS/protocol/layer3.h>

bool DPAUCS_withRawAsLogicAddress( uint16_t type, void* addr, size_t size, void(*func)(const DPAUCS_logicAddress_t*,void*), void* param ){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( type );
  if( handler && handler->withRawAsLogicAddress )
    return (*handler->withRawAsLogicAddress)( addr, size, func, param );
  return false;
}

bool DPAUCS_isBroadcast( const DPAUCS_logicAddress_t* address){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( address->type );
  if( handler && handler->isBroadcast )
    return (*handler->isBroadcast)( address );
  return false;
}

bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t* a,const DPAUCS_logicAddress_t* b){
  if( a->type != b->type )
    return false;
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( a->type );
  if( handler && handler->compare )
    return (*handler->compare)( a, b );
  return false;
}

bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t* address){
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( address->type );
  if( handler && handler->isValid )
    return (*handler->isValid)( address );
  return false;
}

bool DPAUCS_copy_logicAddress( DPAUCS_logicAddress_t* dst, const DPAUCS_logicAddress_t* src ){
  if( dst->type != src->type )
    return false;
  const DPAUCS_l3_handler_t* handler = DPAUCS_getAddressHandler( src->type );
  if( handler && handler->copy )
    return (*handler->copy)( dst, src );
  return false;
}

bool DPAUCS_persistMixedAddress( DPAUCS_mixedAddress_pair_t* map ){
  if( map->flags & DPAUCS_F_AP_PERSISTENT
   || map->type != DPAUCS_AP_ADDRESS
  ) return false;
  const DPAUCS_logicAddress_t* src = DPAUCS_get_logicAddress( &map->address.source->logic );
  if( src ){
    map->logic.source = src;
  }else{
    map->flags |= DPAUCS_F_AP_SRC_SVR_OR_APC;
    map->logic.source = &DPAUCS_arpCache_register( map->address.source )->logic;
  }
  const DPAUCS_logicAddress_t* dest = DPAUCS_get_logicAddress( &map->address.source->logic );
  if( dest ){
    map->logic.destination = dest;
  }else{
    map->flags |= DPAUCS_F_AP_DST_SVR_OR_APC;
    map->logic.destination = &DPAUCS_arpCache_register( map->address.destination )->logic;
  }
  map->type = DPAUCS_AP_LOGIC_ADDRESS;
  map->flags |= DPAUCS_F_AP_PERSISTENT;
  return true;
}

bool DPAUCS_freeMixedAddress( DPAUCS_mixedAddress_pair_t* map ){
  if(!( map->flags & DPAUCS_F_AP_PERSISTENT )
     || map->type != DPAUCS_AP_LOGIC_ADDRESS
  ) return false;
  if(!( map->flags & DPAUCS_F_AP_SRC_SVR_OR_APC ))
    DPAUCS_arpCache_deregister( map->logic.source );
  if(!( map->flags & DPAUCS_F_AP_DST_SVR_OR_APC ))
    DPAUCS_arpCache_deregister( map->logic.destination );
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
  mixed->logic = *address;
  return true;
}

const DPAUCS_logicAddress_t* DPAUCS_mixedPairComponentToLogicAddress(
  const DPAUCS_mixedAddress_pair_t* mixed,
  bool source_or_destination
){
  switch( mixed->type ){
    case DPAUCS_AP_ADDRESS:
      return source_or_destination
       ? &mixed->address.source->logic
       : &mixed->address.destination->logic;
    case DPAUCS_AP_LOGIC_ADDRESS:
      return source_or_destination
       ? mixed->logic.source
       : mixed->logic.destination;
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
                                            ? mixed->logic.source
                                            : mixed->logic.destination;
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
      address->source = &mixed->address.source->logic;
      address->destination = &mixed->address.destination->logic;
    } return true;
    case DPAUCS_AP_LOGIC_ADDRESS: {
      *address = mixed->logic;
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

uint16_t DPAUCS_mixedPairGetType( const DPAUCS_mixedAddress_pair_t* addr ){
  switch( addr->type ){
    case DPAUCS_AP_ADDRESS      : return addr->address.source->type;
    case DPAUCS_AP_LOGIC_ADDRESS: return addr->logic.source->type;
  }
  return 0;
}
