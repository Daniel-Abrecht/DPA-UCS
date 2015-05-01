#include <stdbool.h>
#include <string.h>
#include <eth.h>
#include <server.h>
#include <service.h>
#include <packet.h>
#include <binaryUtils.h>
#include <protocol/ethtypes.h>
#include <protocol/arp.h>
#include <protocol/ip.h>
#include <stdio.h>

uint32_t ips[MAX_IPS] = {0};

static DPAUCS_packet_t packetInputBuffer;
static DPAUCS_packet_t nextPacketToSend;

static struct {
  bool active;
  uint32_t ip;
  uint16_t port;
  DPAUCS_service_t service;
} services[MAX_SERVICES];

void DPAUCS_init( void ){

  memset(services,0,sizeof(services));

  DPAUCS_ethInit(mac);

}

void DPAUCS_shutdown( void ){

}

void DPAUCS_add_ip( uint32_t ip ){
  for(int i=0;i<MAX_IPS;i++){
    if(!ips[i]){
      ips[i] = ip;
      break;
    }
  }
}

void DPAUCS_remove_ip( uint32_t ip ){
  for(int i=0;i<MAX_IPS;i++){
    if(ips[i]==ip){
      ips[i] = 0;
      break;
    }
  }
}

void DPAUCS_add_service( uint32_t ip, uint16_t port, DPAUCS_service_t* service ){
  for(int i=0;i<MAX_SERVICES;i++){
    if(!services[i].active){
      services[i].active = true;
      services[i].port = port;
      services[i].ip = ip;
      services[i].service = *service;
      (*services[i].service.start)();
      break;
    }
  }
}

void DPAUCS_remove_service( uint32_t ip, uint16_t port ){
  for(int i=0;i<MAX_SERVICES;i++){
    if( services[i].ip == ip && services[i].port == port ){
      services[i].active = false;
      (*services[i].service.stop)();
      break;
    }
  }
}

void getPacketInfo( DPAUCS_packet_info* info, DPAUCS_packet_t* packet ){

  memset(info,0,sizeof(*info));

  memcpy(info->destination_mac,packet->data.dest,6);
  memcpy(info->source_mac,packet->data.src,6);

  info->is_vlan = packet->data.vlan.tpid == IEEE_802_1Q_TPID_CONST;

  if( info->is_vlan ){ // maybe an IEEE 802.1Q tag
    info->vid     = btoh16(packet->data.vlan.pcp_dei_vid) & 0x0FFF;
    info->type    = btoh16(packet->data.vlan.type);
    info->payload = packet->data.vlan.payload;
    info->llc     = &packet->data.vlan.llc;
    info->size    = btoh16(packet->data.length);
  }else{
    info->vid     = 0;
    info->type    = btoh16(packet->data.type);
    info->payload = packet->data.payload;
    info->llc     = &packet->data.llc;
    info->size    = btoh16(packet->data.vlan.length);
  }

  if( info->type < 1536 && info->type > 1500 ){ // undefined case
    info->invalid = true;
  }

  if( info->type <= 1500 ){ // field contained length instant of type
    if(info->llc->dsap != 0xAA){ // Cant handle this kind of frame
      info->invalid = true;
      return;
    }else{ // ok, it's snap
      info->type    = btoh16(info->llc->extended_control.snap.type);
      info->payload = info->llc->extended_control.payload;
    }
  }

}


void DPAUCS_doNextTask( void ){
  do { // Recive & handle packet

    DPAUCS_packet_t*const packet = &packetInputBuffer;
    packet->size = DPAUCS_ethReceive(packet->data.raw,PACKET_SIZE);

    if(!packet->size)
      break;

    if(
         memcmp(packet->data.dest,"\xFF\xFF\xFF\xFF\xFF\xFF",6) // not broadcast
      && memcmp(packet->data.dest,mac,6) // not my mac
    ) break; // ignore packet

    DPAUCS_packet_info info;

    getPacketInfo(&info,packet);

    if(info.invalid){
      packet->size = 0; // discard package
      break;
    }

    switch(info.type){
      case ETH_TYPE_ARP: DPAUCS_arp_handler(&info); break;
      case ETH_TYPE_IP_V4: DPAUCS_ip_handler(&info); break;
    }

  } while(0);

}

void DPAUCS_preparePacket( DPAUCS_packet_info* info ){

  DPAUCS_packet_t* packet = &nextPacketToSend;

  memcpy(info->source_mac,mac,6);
  
  memcpy(packet->data.dest,info->destination_mac,6);
  memcpy(packet->data.src,info->source_mac,6);

  if( info->is_vlan ){
    packet->data.vlan.tpid = IEEE_802_1Q_TPID_CONST;
    packet->data.vlan.pcp_dei_vid = htob16(info->vid);
    packet->data.vlan.type = htob16(info->type);
    info->payload = packet->data.vlan.payload;
  }else{
    packet->data.vlan.pcp_dei_vid  = 0;
    packet->data.type      = htob16(info->type);
    info->payload          = packet->data.payload;
  }

  packet->size = (uintptr_t)packet->data.payload - (uintptr_t)packet;

}

void DPAUCS_sendPacket( DPAUCS_packet_info* info, uint16_t size ){
  size += (uint8_t*)info->payload - nextPacketToSend.data.raw; // add ethernetheadersize
  nextPacketToSend.size = size;
  DPAUCS_ethSend( nextPacketToSend.data.raw, size );
}

