#include <protocol/anyAddress.h>

bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t* address){
  switch( address->type ){
    case AT_IPv4: {
      return !~((const DPAUCS_logicAddress_IPv4_t*)address)->address;
    } break;
  }
  return false;
}

bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t* a,const DPAUCS_logicAddress_t* b){
  if( a->type != b->type )
    return false;
  switch(a->type){
    case AT_IPv4: {
      return ((const DPAUCS_logicAddress_IPv4_t*)a)->address == ((const DPAUCS_logicAddress_IPv4_t*)b)->address;
    } break;
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
    case AT_IPv4: {
      *(DPAUCS_logicAddress_IPv4_t*)dst = *(const DPAUCS_logicAddress_IPv4_t*)src;
      return true;
    } break;
  }
  return false;
}
