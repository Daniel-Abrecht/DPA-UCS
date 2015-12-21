/**
* Der meiste Code in dieser Datei, aber nicht der gesammte,
* stammt fon folgendem Tutorial
* http://backreference.org/2010/03/26/tuntap-interface-tutorial/
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     printf("Error: %d %s\n",errno,strerror(errno));
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     printf("Error: %d %s\n",errno,strerror(errno));
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

int main(){
  char buf[1024*1024];
  char name[IFNAMSIZ]="echotap";
  int tap = tun_alloc(name, IFF_TAP);
  if( tap < 0 )
    return 1;
  if(ioctl(tap, TUNSETPERSIST, 1) < 0)
    return 2;
  printf("Device %s\n",name);
  while(1){
    ssize_t size = read(tap,buf,sizeof(buf));
    if( size < 0 )
      printf("Error: %d %s\n",errno,strerror(errno));
    if( write(tap,buf,size) < 0 )
      printf("Error: %d %s\n",errno,strerror(errno));
  }
  close(tap);
  return 0;
}

