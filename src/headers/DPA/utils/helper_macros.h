#ifndef DPA_HELPER_MACROS_H
#define DPA_HELPER_MACROS_H

#if __STDC_VERSION__ < 201112L
#ifndef __GNUC__
#pragma anon_unions
#endif
#ifndef noreturn
#define noreturn __attribute__((noreturn))
#endif
#define static_assert(...)
#else
#include <assert.h>
#include <stdnoreturn.h>
#endif

#if __alignof_is_defined
#include <stdalign.h>
#define DPA_ALIGNOF(T) alignof(T)
#else
#include <stddef.h>
#define DPA_ALIGNOF(T) offsetof(struct{char x;T y;},y)
#endif

#ifndef packed
#define packed __attribute__ ((__packed__))
#endif

#ifndef weak
#define weak __attribute__((weak))
#endif

#define DPA_MODULE( name ) void modul_ ## name( void )
#define DPA_DEPENDENCY( name ) modul_ ## name()

#ifndef flash
#ifdef __FLASH
#define flash __flash
#define PRIsFLASH "S"
#else
#define flash
#define PRIsFLASH "s"
#endif
#endif

#define RESERVED DPA_CONCAT( R_RESERVED_, __LINE__ )
#define RESERVED2 DPA_CONCAT( R_RESERVED2_, __LINE__ )
#define RESERVED3 DPA_CONCAT( R_RESERVED3_, __LINE__ )
#define RESERVED4 DPA_CONCAT( R_RESERVED4_, __LINE__ )

#define DPA_UNPACK( ... ) __VA_ARGS__

#define DPA_CALC_PREV_ALIGN_OFFSET( x, T ) \
  ( (size_t)( x ) & (size_t)~( DPA_ALIGNOF(T) - 1 ) )

#define DPA_CALC_ALIGN_OFFSET( x, T ) \
  DPA_CALC_PREV_ALIGN_OFFSET( (size_t)(x) + DPA_ALIGNOF(T) - 1, T )

#define DPA_STRINGIFY(x) #x
#define DPA_TOSTRING(x) DPA_STRINGIFY(x)

#define DPA_CONCAT_EVAL(A,B) A ## B
#define DPA_CONCAT(A,B) DPA_CONCAT_EVAL(A,B)

#define DPA_LOOSE_LIST_DECLARE(TYPE,NAME) \
  struct NAME; \
  struct NAME { \
    TYPE entry; \
    struct NAME* next; \
  }; \
  extern struct NAME* NAME; \
  weak struct NAME* NAME;

#define DPA_LOOSE_LIST_ADD(LIST,ENTRY) \
  static struct LIST DPA_CONCAT( list_item_, __LINE__ ) = {(ENTRY),0}; \
  static void DPA_CONCAT( list_append_, __LINE__ )() __attribute__((constructor)); \
  static void DPA_CONCAT( list_append_, __LINE__ )(){ \
    DPA_CONCAT( list_item_, __LINE__ ).next = (LIST); \
    LIST = &DPA_CONCAT( list_item_, __LINE__ ); \
  }

#endif
