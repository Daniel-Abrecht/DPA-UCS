#ifndef DPA_ENCODING_BASE64_H
#define DPA_ENCODING_BASE64_H

#include <stddef.h>

size_t DPA_encoding_base64_getLength( size_t n );
void DPA_encoding_base64_encode( void* out, size_t n, const void* in, size_t m );

#endif
