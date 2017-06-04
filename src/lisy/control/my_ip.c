#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
 
int main(int c, char **v) {
 
  struct ifreq ifa;
  struct sockaddr_in *i;
  int fd;
 
  if(c != 2) {
    fprintf(stderr, "Usage: %s <iface>\n", v[0]);
    exit(EXIT_FAILURE);
  }
 
  if (strlen (v[1]) >= sizeof ifa.ifr_name) {
    fprintf (stderr, "%s is to long\n", v[1]);
    exit (EXIT_FAILURE);
  }
 
  strcpy (ifa.ifr_name, v[1]);
 
  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror ("socket");
    exit (EXIT_FAILURE);
  }
 
  if(ioctl(fd, SIOCGIFADDR, &ifa)) {
    perror ("ioctl");
    exit (EXIT_FAILURE);
  }
 
  i = (struct sockaddr_in*)&ifa.ifr_addr;
  puts (inet_ntoa(i->sin_addr));
 
  return 0;
}
