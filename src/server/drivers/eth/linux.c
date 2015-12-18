#include <sys/socket.h>
#include <netinet/in.h>
#include <logger.h>
#include <string.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <server.h>

static int sock;
uint8_t mac[] = {0,0,0,0,0,0};
char ifname[IFNAMSIZ];

static int setIfaceNameMac(){
  char buf[8192] = {0};
  int sck = sock;

  struct ifconf ifc = {0};
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;

  if(ioctl(sck, SIOCGIFCONF, &ifc) < 0){
    DPAUCS_LOG( "error: ioctl: %d %s\n", errno, strerror(errno) );
    return -1;
  }

  /* Iterate through the list of interfaces. */
  struct ifreq* ifr = ifc.ifc_req;
  int nInterfaces = ifc.ifc_len / sizeof(struct ifreq);

  for(int i = 0; i < nInterfaces; i++){
    struct ifreq* item = &ifr[i];

    /* Get the MAC address */
    if(ioctl(sck, SIOCGIFHWADDR, item) < 0) {
      DPAUCS_LOG( "error: ioctl: %d %s\n", errno, strerror(errno) );
      continue;
    }

    if(
        !item->ifr_hwaddr.sa_data[0]
     && !item->ifr_hwaddr.sa_data[1]
     && !item->ifr_hwaddr.sa_data[2]
     && !item->ifr_hwaddr.sa_data[3]
     && !item->ifr_hwaddr.sa_data[4]
     && !item->ifr_hwaddr.sa_data[5]
    ) continue;

    strcpy(ifname,item->ifr_name);
    memcpy(mac,item->ifr_hwaddr.sa_data,6);

    if( ioctl( sck, SIOCGIFINDEX, item ) < 0 )
      DPAUCS_LOG( "error: ioctl %d %s\n", errno, strerror(errno) );
    return item->ifr_ifindex;
  }
  return -1;
}

void DPAUCS_ethInit( uint8_t* macaddr ){

  int ifi;

  (void) macaddr; // unused

  sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if(sock<0){
    DPAUCS_LOG("error: socket: %d %s\n", errno, strerror(errno));
    goto fatal_error;
  }

  {
    int flags = fcntl(sock,F_GETFL,0);
    if(flags<0){
      DPAUCS_LOG("error: fcntl: %d %s\n",errno,strerror(errno));
      goto fatal_error;
    }
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    if(flags==-1){
      DPAUCS_LOG("error: fcntl: %d %s\n",errno,strerror(errno));
      goto fatal_error;
    }
  }

  {
    ifi = setIfaceNameMac();
    if( ifi < 0 )
      goto fatal_error;

    struct sockaddr_ll addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.sll_family   = PF_PACKET;
    addr.sll_protocol = 0;
    addr.sll_ifindex  = ifi;

    if( bind( sock, (const struct sockaddr*)&addr, sizeof( addr ) ) < 0 ){
      DPAUCS_LOG( "error: bind: %d %s\n", errno, strerror(errno) );
      goto fatal_error;
    }
  }
    /* display result */

  DPAUCS_LOG("%s %02x:%02x:%02x:%02x:%02x:%02x\n", 
    ifname,
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]
  );

  {
    struct packet_mreq mr;
    memset ( &mr, 0, sizeof(mr) );
    mr.mr_ifindex = ifi;
    mr.mr_type = PACKET_MR_PROMISC;
    if( setsockopt (sock, SOL_PACKET,PACKET_ADD_MEMBERSHIP, &mr, sizeof (mr)) == -1 )
      DPAUCS_LOG( "error: init: setsockopt 2 failed, %d %s\n", errno, strerror(errno) ); // not fatal
  }

  return;

  fatal_error: DPAUCS_fatal( "DPAUCS_ethInit failed\n" );

}

void DPAUCS_ethSend( uint8_t* packet, uint16_t len ){
  long ret = send(sock, packet, len, 0);
  if( ret == len ){
    DPAUCS_LOG( "send: %lu OK\n", (unsigned long)len );
  }else if(ret<0){
    DPAUCS_LOG( "send: error: %d %s\n", errno, strerror(errno) );
  }else{
    DPAUCS_LOG( "send: error: only %ld of %ld bytes sent\n", (long)ret, (unsigned long)len );
  }
}

uint16_t DPAUCS_ethReceive( uint8_t* packet, uint16_t maxlen ){
  long i = recv(sock, packet, maxlen, 0);
  if( i<0 && errno != EAGAIN && errno != EWOULDBLOCK )
    DPAUCS_LOG( "recv: %d %s\n", errno, strerror(errno) );
  return i < 0 ? 0 : i;
}

void DPAUCS_ethShutdown(){
  close(sock);
}
