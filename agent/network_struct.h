#ifndef NETWORK_STRUCT_H_
#define NETWORK_STRUCT_H_

#define ETHERNET_LEN 14
#define ETHERNET_ADDR_LEN 6

struct ethernet_header {
  u_char  dest_host[ETHERNET_ADDR_LEN];
  u_char  source_host[ETHERNET_ADDR_LEN];
  u_short type;
};

struct ip_header {
  u_char          v_ihl;
  u_char          tos;
  u_short         len;
  u_short         id;
  u_short         offset;
  u_char          ttl;
  u_char          protocol;
  u_short         checksum;
  struct in_addr  src_ip_addr;
  struct in_addr  dst_ip_addr;
};

#define IP_IHL(ip) ((ip->v_ihl) & 0x0f)
#define IP_V(ip) ((ip->v_ihl) >> 4)

struct tcp_header {
  u_short src_port;
  u_short dst_port;
  u_int   seq_num;
  u_int   ack_num;
  u_char  offset;
  u_char  flags;
  u_short window;
  u_short checksum;
  u_short urg_ptr;
};

#define TH_OFF(th) ((th->offset & 0xf0) >> 4)

#endif
