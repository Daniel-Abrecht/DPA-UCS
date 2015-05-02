#ifndef TCP_H
#define TCP_H

#define IP_PROTOCOL_TCP 6

#include <stdbool.h>
#include <packed.h>
#include <protocol/ip.h>

void DPAUCS_tcpInit();
void DPAUCS_tcpShutdown();

#endif
