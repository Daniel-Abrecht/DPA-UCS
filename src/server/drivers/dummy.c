#include <server.h>

void DPAUCS_ethInit( uint8_t* macaddr ){
  (void)macaddr;
}

void DPAUCS_ethSend( uint8_t* packet, uint16_t len ){
  (void)packet;
  (void)len;
}

uint16_t DPAUCS_ethReceive( uint8_t* packet, uint16_t maxlen ){
  (void)packet;
  (void)maxlen;
  return 0;
}

void DPAUCS_ethShutdown(){
}
