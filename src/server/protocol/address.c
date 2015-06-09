#include <protocol/address.h>

bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t* address){
  switch( address->type ){
    case AT_IPv4: {
      return !~((DPAUCS_logicAddress_IPv4_t*)address)->address;
    } break;
  }
  return false;
}

bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t* a,const DPAUCS_logicAddress_t* b){
  if( a->type != b->type )
    return false;
  switch(a->type){
    case AT_IPv4: {
      return ((DPAUCS_logicAddress_IPv4_t*)a)->address == ((DPAUCS_logicAddress_IPv4_t*)b)->address;
    } break;
  }
  return false;
}

bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t* address){
  return !DPAUCS_isBroadcast(address);
}
