#ifndef DPAUCS_ADDRESS_TYPES_H
#define DPAUCS_ADDRESS_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <DPA/utils/helper_macros.h>

#define DPAUCS_MAC( a,b,c,d,e,f ) {0x##a,0x##b,0x##c,0x##d,0x##e,0x##f}
#define DPAUCS_ANY_ADDRESS 0
#define DPAUCS_GET_ADDR( X ) ((char*)( &(X)->type + 1 ))

#define DPAUCS_LA( TYPE, SIZE, ADDRESS ) \
  (DPAUCS_logicAddress_t*)((struct { \
    uint16_t type; \
    unsigned char address[SIZE]; \
  }[]){{ \
    .type = TYPE, \
    .address = { DPA_UNPACK ADDRESS } \
  }})

#define DPAUCS_ADDR( TYPE, SIZE, MAC, LA ) \
  (DPAUCS_address_t*)((struct { \
    unsigned char mac[6]; \
    uint16_t type; \
    unsigned char address[SIZE]; \
  }[]){{ \
    .mac = { DPA_UNPACK MAC }, \
    .type = TYPE, \
    .address = { DPA_UNPACK LA } \
  }})

typedef uint8_t DPAUCS_mac_t[6];

typedef struct DPAUCS_interface {
  DPAUCS_mac_t mac;
  bool link_up;
} DPAUCS_interface_t;

// Don't change this struct
typedef struct DPAUCS_logicAddress {
  uint16_t type;
} DPAUCS_logicAddress_t;

typedef struct DPAUCS_address {
  DPAUCS_mac_t mac;
  union {
    uint16_t type;
    DPAUCS_logicAddress_t logic;
  };
} DPAUCS_address_t;

typedef struct DPAUCS_logicAddress_pair {
  const struct DPAUCS_logicAddress* source;
  const struct DPAUCS_logicAddress* destination;
} DPAUCS_logicAddress_pair_t;

typedef struct DPAUCS_address_pair {
  struct DPAUCS_address* source;
  struct DPAUCS_address* destination;
} DPAUCS_address_pair_t;

enum DPAUCS_mixedAddress_pair_type {
  DPAUCS_AP_ADDRESS,
  DPAUCS_AP_LOGIC_ADDRESS
};

enum ap_flags {
 DPAUCS_F_AP_PERSISTENT     = 1<<0,
 DPAUCS_F_AP_SRC_SVR_OR_APC = 1<<1,
 DPAUCS_F_AP_DST_SVR_OR_APC = 1<<2
};

typedef struct DPAUCS_mixedAddress_pair {
  union {
    DPAUCS_address_pair_t address;
    DPAUCS_logicAddress_pair_t logic;
  };
  enum DPAUCS_mixedAddress_pair_type type;
  uint8_t flags;
} DPAUCS_mixedAddress_pair_t;

#define DPAUCS_TYPE_CHECK(T,V) \
  (void)((struct{T x;}){.x=(V)})

#define DPAUCS_logicAddress_pair_t(T,S,D) \
  (const DPAUCS_logicAddress_pair_t){ \
    .source = ( \
      DPAUCS_TYPE_CHECK(const DPAUCS_logicAddress_ ## T ## _t*,(S)), \
      &(S)->logicAddress \
    ), \
    .destination = ( \
      DPAUCS_TYPE_CHECK(const DPAUCS_logicAddress_ ## T ## _t*,(D)), \
      &(D)->logicAddress \
    ) \
  }

#define DPAUCS_address_pair_t(T,S,D) \
  (const DPAUCS_address_pair_t){ \
    .source = ( \
      DPAUCS_TYPE_CHECK(const DPAUCS_ ## T ## _address_t*,(S)), \
      &(S)->logicAddress \
    ), \
    .destination = ( \
      DPAUCS_TYPE_CHECK(const DPAUCS_ ## T ## _address_t*,(D)), \
      &(D)->logicAddress \
    ) \
  }

bool DPAUCS_persistMixedAddress( DPAUCS_mixedAddress_pair_t* );
bool DPAUCS_freeMixedAddress( DPAUCS_mixedAddress_pair_t* );

bool DPAUCS_addressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_address_pair_t* address );
void DPAUCS_swapMixedAddress( DPAUCS_mixedAddress_pair_t* mixed );
bool DPAUCS_logicAddressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_logicAddress_pair_t* address );
bool DPAUCS_mixedPairToAddress( DPAUCS_address_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed );
bool DPAUCS_mixedPairToLogicAddress( DPAUCS_logicAddress_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed );
bool DPAUCS_mixedPairEqual( const DPAUCS_mixedAddress_pair_t*, const DPAUCS_mixedAddress_pair_t* );
uint16_t DPAUCS_mixedPairGetType( const DPAUCS_mixedAddress_pair_t* );
const DPAUCS_logicAddress_t* DPAUCS_mixedPairComponentToLogicAddress(
  const DPAUCS_mixedAddress_pair_t* mixed,
  bool source_or_destination
);
const DPAUCS_address_t* DPAUCS_mixedPairComponentToAddress(
  const DPAUCS_mixedAddress_pair_t* mixed,
  bool source_or_destination
);
bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t*);
bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t*,const DPAUCS_logicAddress_t*);
bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t*);
bool DPAUCS_copy_logicAddress( DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t* );
bool DPAUCS_withRawAsLogicAddress( uint16_t, void*, size_t, void(*)(const DPAUCS_logicAddress_t*,void*), void* );

#endif
