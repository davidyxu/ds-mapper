#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

struct in_addr get_dev_addr(const char * const dev)
{
    int fd;
    struct ifreq addr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Type of address to retrieve - IPv4 IP address
    addr.ifr_addr.sa_family = AF_INET;

    // Copy the interface name in the ifreq structure
    strncpy(addr.ifr_name , dev , IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &addr);
    close(fd);

    return ((struct sockaddr_in *)&addr.ifr_addr )->sin_addr;
}
