#ifndef ADDRESS_TYPES_H
#define ADDRESS_TYPES_H

#include <DPA/UCS/helper_macros.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define STRINGIFY(x) #x

#define IPSTRING( a,b,c,d ) #a "." #b "." #c "." #d
#define IPCONST( a,b,c,d ) {a,b,c,d}
#define IPINT( a,b,c,d ) \
   ((uint32_t)a<<24) \
 | ((uint32_t)b<<16) \
 | ((uint32_t)c<<8) \
 | ((uint32_t)d)

#define MACSTRING( a,b,c,d,e,f ) #a ":" #b ":" #c ":" #d ":" #e ":" #f
#define MACCONST( a,b,c,d,e,f ) {0x##a,0x##b,0x##c,0x##d,0x##e,0x##f}

#define LA_IPv4_INIT \
  .type = AT_IPv4

#define LA_IPv4( a,b,c,d ) { \
    LA_IPv4_INIT, \
    .address = IPINT( a,b,c,d ) \
  }

#define DPAUCS_LA_SIZE( type ) ( \
    (type) == AT_IPv4 ? 4 : \
    0 \
  )

#define ANY_ADDRESS 0
#define AT_LAYER3 ( AT_IPv4 | AT_IPv6 )

typedef enum {
#ifdef USE_IPv4
  AT_IPv4 = 1<<0,
#endif
#ifdef USE_IPv6
  AT_IPv6 = 1<<1,
#endif
} DPAUCS_address_types_t;

typedef struct {
  DPAUCS_address_types_t type;
} DPAUCS_logicAddress_t;

typedef struct {
  uint8_t mac[6];
  union {
    DPAUCS_address_types_t type;
    DPAUCS_logicAddress_t logicAddress;
  };
} DPAUCS_address_t;

typedef struct {
  const DPAUCS_logicAddress_t* source;
  const DPAUCS_logicAddress_t* destination;
} DPAUCS_logicAddress_pair_t;

typedef struct {
  DPAUCS_address_t* source;
  DPAUCS_address_t* destination;
} DPAUCS_address_pair_t;

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

bool DPAUCS_isBroadcast(const DPAUCS_logicAddress_t*);
bool DPAUCS_compare_logicAddress(const DPAUCS_logicAddress_t*,const DPAUCS_logicAddress_t*);
bool DPAUCS_isValidHostAddress(const DPAUCS_logicAddress_t*);
bool DPAUCS_copy_logicAddress( DPAUCS_logicAddress_t*, const DPAUCS_logicAddress_t* );

#endif