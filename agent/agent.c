#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <ifaddrs.h>

#include "dev_addr.h"
#include "http_post.h"
#include "json_event.h"
#include "pcap_conf.h"
#include "network_struct.h"

#define MAX_URL_LEN 1024
#define SNAP_LEN 16 * 1024
#define BATCH_LEN 64 * 1024

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

const int get_http_code(const u_char *payload, int len);
const char * get_http_method(const u_char *payload, int len);
void get_ascii_payload(char *buffer, const u_char *payload, int len);

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

  static char batch_buf[BATCH_LEN];
  static int batch_offset = 0;

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

  const char *http_method = get_http_method(payload, payload_len);
  const int http_code = get_http_code(payload, payload_len);

  if (!http_method && !http_code)
    return;

  /*
  const char *direction;
  if (ip->src_ip_addr.s_addr == conf->dev_addr.s_addr)
    direction = "out";
  else if (ip->dst_ip_addr.s_addr == conf->dev_addr.s_addr)
    direction = "in";

  printf("\n\nPacket #%d:\n", ++count);
  printf("Packet ID:    %hu\n", ip->id);
  printf("From:         %s:%d\n", inet_ntoa(ip->src_ip_addr), ntohs(tcp->src_port));
  printf("To:           %s:%d\n", inet_ntoa(ip->dst_ip_addr), ntohs(tcp->dst_port));
  printf("Seq #:        %u\n", tcp->seq_num);
  printf("Ack #:        %u\n", tcp->ack_num);

  if (http_method)
    printf("HTTP Method:  %s\n", http_method);
  if (http_code)
    printf("HTTP Code:    %d\n", http_code);

  printf("Payload Size: %d bytes\n", payload_len);

  */
  char payload_buffer[(2 * payload_len) + 1]; /* use double payload_len in case of escaped characters */
  get_ascii_payload(payload_buffer, payload, payload_len);

  const u_int buf_len = SNAP_LEN + 1024; /* give extra space for json formatting */
  char event_buf[buf_len];
  int event_offset = 0;

  /* build event json */
  event_offset = append_event_json_int(event_buf, buf_len, event_offset, "timestamp", (int)time(NULL));
  event_offset = append_event_json_str(event_buf, buf_len, event_offset, "src_ip", inet_ntoa(ip->src_ip_addr));
  event_offset = append_event_json_int(event_buf, buf_len, event_offset, "src_port", ntohs(tcp->src_port));
  event_offset = append_event_json_str(event_buf, buf_len, event_offset, "dst_ip", inet_ntoa(ip->dst_ip_addr));
  event_offset = append_event_json_int(event_buf, buf_len, event_offset, "dst_port", ntohs(tcp->dst_port));

  if (http_method)
    event_offset = append_event_json_str(event_buf, buf_len, event_offset, "http_method", http_method);
  if (http_code)
    event_offset = append_event_json_int(event_buf, buf_len, event_offset, "http_code", http_code);

  event_offset = append_event_json_str(event_buf, buf_len, event_offset, "payload", payload_buffer);
  event_offset = close_event_json(event_buf, buf_len, event_offset);

  /* append to batch json */
  batch_offset = append_batch_event(batch_buf, BATCH_LEN, batch_offset, event_buf, event_offset, conf);

  return;
}

int main(int argc, char **argv)
{
  struct in_addr dev_addr;
  char *dev = NULL;


  char event_url[MAX_URL_LEN];
  char register_url[MAX_URL_LEN];


  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;

  struct bpf_program fp;

  bpf_u_int32 mask;
  bpf_u_int32 net;

  if (argc < 2) {
    printf("usage: agent <url> <dev>\n");
    return 1;
  } else {
    sprintf(event_url, "http://%s/event", argv[1]);
    sprintf(register_url, "http://%s/register", argv[1]);
  }
  if (argc == 3) {
    dev = argv[2];
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
