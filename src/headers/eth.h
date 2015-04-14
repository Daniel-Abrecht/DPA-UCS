#ifndef ETH_H
#define ETH_H

#include <stdint.h>

void DPAUCS_ethInit( uint8_t* macaddr );
void DPAUCS_ethSend( uint8_t* packet, uint16_t len );
uint16_t DPAUCS_ethReceive( uint8_t* packet, uint16_t maxlen );

#endif
