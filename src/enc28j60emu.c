#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_packet.h>

static int sock;

void enc28j60Init( unsigned char* macaddr ){
  printf("socket: ");
  fflush(stdout);
  sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  printf(sock>=0?"ok\n":"nope\n");
  int flags = fcntl(sock,F_GETFL,0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

//  char dev[] = "wlan1";
//  setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, dev, sizeof(dev)-1);

  char* ifname = "wlan1";
  
  struct ifreq req;
  memset( &req, 0, sizeof( req ) );
  strcpy( (char*)req.ifr_name, (char*)ifname );

  if( ioctl( sock, SIOCGIFINDEX, &req ) < 0 ){
    printf( "init: ioctl SIOCGIFINDEX failed" );
    return;
  }

  struct sockaddr_ll addr;
  memset( &addr, 0, sizeof( addr ) );
  addr.sll_family   = PF_PACKET;
  addr.sll_protocol = 0;
  addr.sll_ifindex  = req.ifr_ifindex;

  if( bind( sock, (const struct sockaddr*)&addr, sizeof( addr ) ) < 0 ){
    printf( "init: bind failed" );
    return;
  }

  if( ioctl( sock, SIOCGIFHWADDR, &req ) < 0 ){
    printf( "init: ioctl SIOCGIFHWADDR failed " );
    return;
  }

  int on = 1;
  setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
  (void) macaddr;
}

void enc28j60PacketSend(unsigned int len, unsigned char *packet){
  printf("send: %lu ",(unsigned long)len);
  fflush(stdout);
  long ret = send(sock, packet, len, 0);
  printf("%ld %s\n",(long)ret,ret==len?"ok":"nope");
}

unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char *packet){
  long i = recv(sock, packet, maxlen, 0);
  return i<0?0:i;
}

