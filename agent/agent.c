#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ifaddrs.h>

#include "dev_addr.h"
#include "http_post.h"

#define MAX_URL_LEN 1024
#define SNAP_LEN 16 * 1024
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

struct pcap_conf {
  const struct in_addr dev_addr;
  char url[1024];
};

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

const int get_http_code(const u_char *payload, int len);
const char * get_http_method(const u_char *payload, int len);
void get_ascii_payload(char *buffer, const u_char *payload, int len);

const int append_json_str(char *buffer, const u_int buf_len, int offset, const char *key, const char *value);
const int append_json_int(char *buffer, const u_int buf_len, int offset, const char *key, const u_int value);
const int setup_json_for_append(char *buffer, const u_int buf_len, int offset);
const int close_json(char *buffer, const u_int buf_len, int offset);

const int append_json_str(char *buffer, const u_int buf_len, int offset, const char *key, const char *value)
{
  offset = setup_json_for_append(buffer, buf_len, offset);
  offset += snprintf(buffer + offset, buf_len - offset, "\"%s\":\"%s\"", key, value);

  return offset;
}

const int append_json_int(char *buffer, const u_int buf_len, int offset, const char *key, const u_int value)
{
  offset = setup_json_for_append(buffer, buf_len, offset);
  offset += snprintf(buffer +offset, buf_len - offset, "\"%s\":%d", key, value);

  return offset;
}

const int setup_json_for_append(char *buffer, const u_int buf_len, int offset)
{
  if (offset == 0)
    offset += snprintf(buffer, buf_len, "{");
  else
    offset += snprintf(buffer + offset, buf_len - offset, ",");

  return offset;
}

const int close_json(char *buffer, const u_int buf_len, int offset)
{
  if (offset == 0)
    offset += snprintf(buffer, buf_len, "{}\0");
  else
    offset += snprintf(buffer + offset, buf_len - offset, "}\0");

  return offset;
}

const int get_http_code(const u_char *payload, int len)
{
  if (len >= 12 && (strncmp("HTTP/", payload, 5) == 0)) {
    char http_code[5];
    memcpy(http_code, &payload[9], 4);
    http_code[5] = '\0';
    return atoi(http_code);
  }
  return 0;
}

const char * get_http_method(const u_char *payload, int len)
{
  if (len >= 7) {
    if (!strncmp("GET", payload, 3)) {
      return "GET";
    } else if (!strncmp("PUT", payload, 3)) {
      return "GET";
    } else if (!strncmp("POST", payload, 4)) {
      return "POST";
    } else if (!strncmp("HEAD", payload, 4)) {
      return "HEAD";
    } else if (!strncmp("TRACE", payload, 5)) {
      return "TRACE";
    } else if (!strncmp("DELETE", payload, 6)) {
      return "DELETE";
    } else if (!strncmp("CONNECT", payload, 7)) {
      return "CONNECT";
    } else if (!strncmp("OPTIONS", payload, 7)) {
      return "OPTIONS";
    }
  }

  return NULL;
}

void get_ascii_payload(char *buffer, const u_char *payload, int len)
{
  int i;
  const char *pay_ptr = payload;
  char *buf_ptr = buffer;

  for (i = 0; i < len; ++i) {
    if (isprint(*pay_ptr)) {
      /* escape quotes */
      if (*pay_ptr == '\"' || *pay_ptr == '\\') {
        *buf_ptr = '\\';
        ++buf_ptr;
      }

      *buf_ptr = *pay_ptr;
      ++buf_ptr;
    } else if (*pay_ptr == '\n') {
      *buf_ptr = '\\';
      ++buf_ptr;
      *buf_ptr = 'n';
      ++buf_ptr;
    } else if (*pay_ptr != '\r') {
      *buf_ptr = '.';
      ++buf_ptr;
    }
    ++pay_ptr;
  }

  *buf_ptr = '\0';

  return;
}

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
  static const struct pcap_conf *conf;
  conf = (const struct pcap_conf *)args;
  static int count = 1;

  const struct ethernet_header *ethernet;
  const struct ip_header *ip;
  const struct tcp_header *tcp;
  const char *payload;

  int ip_len;
  int tcp_len;
  int payload_len;

  ip = (struct ip_header *)(packet + ETHERNET_LEN);
  ip_len = IP_IHL(ip) * 4;
  if (ip_len < 20)
    return;

  tcp = (struct tcp_header *)(packet + ETHERNET_LEN + ip_len);
  tcp_len = TH_OFF(tcp) * 4;
  if (tcp_len < 20)
    return;

  payload = (u_char *)(packet + ETHERNET_LEN + ip_len + tcp_len);
  payload_len = ntohs(ip->len) - ip_len + tcp_len;
  if (payload_len == 0)
    return;

  const char *direction;
  const char *http_method = get_http_method(payload, payload_len);
  const int http_code = get_http_code(payload, payload_len);

  if (!http_method && !http_code)
    return;

  if (ip->src_ip_addr.s_addr == conf->dev_addr.s_addr)
    direction = "out";
  else if (ip->dst_ip_addr.s_addr == conf->dev_addr.s_addr)
    direction = "in";
  else
    direction = "unknown";

  printf("\n\nPacket #%d:\n", ++count);
  printf("Packet ID:    %hu\n", ip->id);
  printf("Direction:    %s\n", direction);
  printf("From:         %s:%d\n", inet_ntoa(ip->src_ip_addr), ntohs(tcp->src_port));
  printf("To:           %s:%d\n", inet_ntoa(ip->dst_ip_addr), ntohs(tcp->dst_port));
  printf("Seq #:        %u\n", tcp->seq_num);
  printf("Ack #:        %u\n", tcp->ack_num);

  if (http_method)
    printf("HTTP Method   %s\n", http_method);
  if (http_code)
    printf("HTTP Code     %d\n", http_code);

  printf("\nPayload (%d bytes):\n", payload_len);

  char payload_buffer[(2 * payload_len) + 1]; /* use double payload_len in case of escaped characters */
  get_ascii_payload(payload_buffer, payload, payload_len);

  const u_int buf_len = SNAP_LEN + 1024; /* give extra space for json formatting */
  char json_buffer[buf_len];
  int offset = 0;

  offset = append_json_str(json_buffer, buf_len, offset, "direction", direction);
  offset = append_json_str(json_buffer, buf_len, offset, "src_ip", inet_ntoa(ip->src_ip_addr));
  offset = append_json_int(json_buffer, buf_len, offset, "src_port", ntohs(tcp->src_port));
  offset = append_json_str(json_buffer, buf_len, offset, "dst_ip", inet_ntoa(ip->dst_ip_addr));
  offset = append_json_int(json_buffer, buf_len, offset, "dst_port", ntohs(tcp->dst_port));
  offset = append_json_str(json_buffer, buf_len, offset, "payload", payload_buffer);
  offset = close_json(json_buffer, buf_len, offset);

  printf("\n\nJSON: %s\n\n", json_buffer);
  printf("%s\n\n\n", conf->url);
  curl_post(conf->url, json_buffer);

  return;
}

int main(int argc, char **argv)
{
  struct in_addr dev_addr;
  char *dev = NULL;

  char *host_url = "33.33.33.10:9999";

  char event_url[MAX_URL_LEN];
  char register_url[MAX_URL_LEN];

  sprintf(event_url, "http://%s/event", host_url);
  sprintf(register_url, "http://%s/register", host_url);

  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;

  struct bpf_program fp;

  bpf_u_int32 mask;
  bpf_u_int32 net;

  if (argc == 2) {
    dev = argv[1];
  } else {
    dev = pcap_lookupdev(pcap_errbuf);
    if (dev == NULL) {
      fprintf(stderr, "Couldn't find default device: %s\n", pcap_errbuf);
      return 1;
    }
  }

  handle = pcap_open_live(dev, SNAP_LEN, 0, 1000, pcap_errbuf);
  if (handle == NULL) {
    fprintf(stderr, "Failed to open device %s...\n", dev);
    return 1;
  }

  if (pcap_lookupnet(dev, &net, &mask, pcap_errbuf) == -1) {
    fprintf(stderr, "Couldn't get netmask for device: %s\n", pcap_errbuf);
    net = 0;
    mask = 0;
  }

  if (pcap_datalink(handle) != DLT_EN10MB) {
    fprintf(stderr, "Not an ethernet device %s...\n", dev);
    return 1;
  }

  dev_addr = get_dev_addr(dev);

  printf("Device: %s\n", dev);
  printf("IP:     %s\n", inet_ntoa(dev_addr));

  if (pcap_compile(handle, &fp, "tcp", 1, net) == -1) {
    fprintf(stderr, "Couldn't parse filter...");
    return 1;
  }

  if (pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter: %s\n", pcap_geterr(handle));
    return 1;
  }

  struct pcap_conf conf = { dev_addr };
  memcpy(conf.url, event_url, MAX_URL_LEN);

  pcap_loop(handle, -1, handle_packet, (u_char *)&conf);

  return 0;
}
