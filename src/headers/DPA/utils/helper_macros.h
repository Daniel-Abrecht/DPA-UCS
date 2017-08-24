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

#define DPA_UNPACK( ... ) __VA_ARGS__

#define DPA_CALC_PREV_ALIGN_OFFSET( x, T ) \
  ( (size_t)( x ) & (size_t)~( DPA_ALIGNOF(T) - 1 ) )

#define DPA_CALC_ALIGN_OFFSET( x, T ) \
  DPA_CALC_PREV_ALIGN_OFFSET( (size_t)(x) + DPA_ALIGNOF(T) - 1, T )

#define DPA_STRINGIFY(x) #x
#define DPA_TOSTRING(x) DPA_STRINGIFY(x)

#define DPA_FOR_SECTION_LIST_HACK(TYPE,NAME,ITERATOR) \
  extern TYPE weak __start_ ## NAME ## _section_list_hack[]; \
  extern TYPE weak __stop_ ## NAME ## _section_list_hack[]; \
  for( TYPE* ITERATOR = __start_ ## NAME ## _section_list_hack; \
       ITERATOR < __stop_ ## NAME ## _section_list_hack; ITERATOR++ )

#define DPA_FOR_SECTION_GET_LIST(TYPE,NAME,START,END) \
  extern TYPE weak __start_ ## NAME ## _section_list_hack[]; \
  extern TYPE weak __stop_ ## NAME ## _section_list_hack[]; \
  static TYPE* START = __start_ ## NAME ## _section_list_hack; \
  static TYPE* END = __stop_ ## NAME ## _section_list_hack;

#define DPA_SECTION_LIST_ENTRY_HACK(TYPE,NAME,SYMBOL) \
  extern TYPE SYMBOL; TYPE SYMBOL \
  __attribute__ ((section ( DPA_TOSTRING( NAME ## _section_list_hack )),used)) =

#endif
