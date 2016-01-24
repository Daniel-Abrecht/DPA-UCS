#ifndef ADDRESS_TYPES_H
#define ADDRESS_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <DPA/UCS/helper_macros.h>

#define DPAUCS_MAC( a,b,c,d,e,f ) {0x##a,0x##b,0x##c,0x##d,0x##e,0x##f}

#define DPAUCS_LA_IPv4_INIT \
  .type = DPAUCS_AT_IPv4

#define DPAUCS_LA_IPv4( a,b,c,d ) { \
    DPAUCS_LA_IPv4_INIT, \
    .address = ((uint32_t)a<<24) \
             | ((uint32_t)b<<16) \
             | ((uint32_t)c<<8) \
             | ((uint32_t)d) \
  }

#define DPAUCS_LA_SIZE( type ) ( \
    (type) == AT_IPv4 ? 4 : \
    0 \
  )

#define DPAUCS_ANY_ADDRESS 0
#define DPAUCS_AT_LAYER3 ( AT_IPv4 | AT_IPv6 )

enum DPAUCS_address_types {
#ifdef USE_IPv4
  DPAUCS_AT_IPv4 = 1<<0,
#endif
#ifdef USE_IPv6
  DPAUCS_AT_IPv6 = 1<<1,
#endif
};

typedef struct DPAUCS_logicAddress {
  enum DPAUCS_address_types type;
} DPAUCS_logicAddress_t;

typedef struct DPAUCS_address {
  uint8_t mac[6];
  union {
    enum DPAUCS_address_types type;
    DPAUCS_logicAddress_t logicAddress;
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

typedef struct DPAUCS_mixedAddress_pair {
  union {
    DPAUCS_address_pair_t address;
    DPAUCS_logicAddress_pair_t logicAddress;
  };
  enum DPAUCS_mixedAddress_pair_type type;
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
void DPAUCS_freeMixedAddress( DPAUCS_mixedAddress_pair_t* );

bool DPAUCS_addressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_address_pair_t* address );
bool DPAUCS_logicAddressPairToMixed( DPAUCS_mixedAddress_pair_t* mixed, const DPAUCS_logicAddress_pair_t* address );
bool DPAUCS_mixedPairToAddress( DPAUCS_address_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed );
bool DPAUCS_mixedPairToLocalAddress( DPAUCS_logicAddress_pair_t* address, const DPAUCS_mixedAddress_pair_t* mixed );
bool DPAUCS_mixedPairEqual( const DPAUCS_mixedAddress_pair_t*, const DPAUCS_mixedAddress_pair_t* );
enum DPAUCS_address_types DPAUCS_mixedPairGetType( const DPAUCS_mixedAddress_pair_t* );

bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t*);
bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t*,const DPAUCS_logicAddress_t*);
bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t*);
bool DPAUCS_copy_logicAddress( DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t* );

#endif