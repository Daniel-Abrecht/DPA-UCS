#ifndef BINARY_UTILS_H
#define BINARY_UTILS_H

#include <stdint.h>

uint16_t btoh16(uint16_t); // big endian to host byte order
uint32_t btoh32(uint32_t); // big endian to host byte order

#define htob16(x) btoh16(x)
#define htob32(x) btoh32(x)

#endif
