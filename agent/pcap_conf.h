#ifndef PCAP_CONF_H_
#define PCAP_CONF_H_

#include <netinet/in.h>

struct pcap_conf {
  const struct in_addr dev_addr;
  char url[1024];
};

#endif
