#include "pcap_conf.h"

int match_services(const struct pcap_conf * const conf, struct in_addr addr, u_short port)
{
  if (addr.s_addr != conf->dev_addr.s_addr)
    return 0;

  int i;
  for (i = 0; i < conf->service_len; ++i) {
    if (conf->services[i].port == port)
      return 1;
  };

  return 0;
}
