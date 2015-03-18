#ifndef PCAP_CONF_H_
#define PCAP_CONF_H_

#define PCAP_URL_LEN 128
#define SERVICE_NAME_LEN 128
#define MAX_SERVICES 24

#include <netinet/in.h>

struct service {
  u_short port;
  char name[SERVICE_NAME_LEN];
};

struct pcap_conf {
  struct in_addr dev_addr;
  struct service services[MAX_SERVICES];
  int service_len;
  char url[PCAP_URL_LEN];
};

int match_services(const struct pcap_conf * const conf, struct in_addr addr, u_short port);

#endif
