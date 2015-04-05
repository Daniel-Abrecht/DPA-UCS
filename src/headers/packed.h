#ifndef PACKED_H
#define PACKED_H

#if __STDC_VERSION__ < 201112L
#pragma anon_unions
#endif

#define PACKED1
#define PACKED2 __attribute__ ((__packed__))

#endif
