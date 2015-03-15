#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <ifaddrs.h>

#include "network_struct.h"
#include "pcap_conf.h"
#include "dev_addr.h"
#include "http_packet_parser.h"
#include "http_post.h"
#include "json_event.h"
#include "json_batch.h"

#define SNAP_LEN 16 * 1024
#define BATCH_LEN 64 * 1024

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

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

  payload = (char *)(packet + ETHERNET_LEN + ip_len + tcp_len);
  payload_len = ntohs(ip->len) - ip_len + tcp_len;
  if (payload_len == 0)
    return;

  // ensure it's the start of a http packet
  if (!is_http_packet(payload, payload_len))
    return;

  // ensure it's a relevant packet (registered service)
  if (!match_services(conf, ip->src_ip_addr, tcp->src_port) && !match_services(conf, ip->dst_ip_addr, tcp->dst_port))
    return;

  printf("\n\nPacket #%d:\n", ++count);
  printf("Packet ID:    %hu\n", ip->id);
  printf("From:         %s:%d\n", inet_ntoa(ip->src_ip_addr), ntohs(tcp->src_port));
  printf("To:           %s:%d\n", inet_ntoa(ip->dst_ip_addr), ntohs(tcp->dst_port));
  printf("Seq #:        %u\n", tcp->seq_num);
  printf("Ack #:        %u\n", tcp->ack_num);
  printf("Payload Size: %d bytes\n", payload_len);


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
  event_offset = append_event_json_str(event_buf, buf_len, event_offset, "payload", payload_buffer);
  event_offset = close_event_json(event_buf, buf_len, event_offset);

  /* append to batch json */
  batch_offset = append_batch_event(batch_buf, BATCH_LEN, batch_offset, event_buf, event_offset, conf);

  return;
}

int main(int argc, char **argv)
{
  struct pcap_conf conf;
  char *dev = NULL;

  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;

  struct bpf_program fp;

  bpf_u_int32 mask;
  bpf_u_int32 net;

  if (argc < 2) {
    /* require at least a processing server url to be provided*/
    printf("usage: agent <url> [<port>=<service>...] <dev>\n");
    return 1;
  } else {
    snprintf(conf.url, PCAP_URL_LEN, "http://%s/event", argv[1]);
  }

  if (argc > 2) {
    conf.service_len = 0;
    int i;
    long int port;
    char * str;
    for (i = 2; i < argc; ++i) {
      port = strtol(argv[i], &str, 10);

      if (port) {
        /* parse as service entry */
        if (*str == '\0' || *str != '=' || *(str+1) == '\0') {
          printf("Specify the name of the service\n");
          return 1;
        } else {
          printf("Registering port %ld as \'%s\'\n", port, str+1);
          conf.services[conf.service_len].port = port;
          snprintf(conf.services[conf.service_len].name, SERVICE_NAME_LEN, "%s", str+1);
          ++conf.service_len;
        }
      } else {
        /* parse as device */
        if (dev) {
          printf("Only one device can be specified!\n");
          return 1;
        } else {
          dev = str;
        }
      }
    }
  }

  /* attempt to find device if none provided */
  if (!dev) {
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

  conf.dev_addr = get_dev_addr(dev);

  printf("Device: %s\n", dev);
  printf("IP:     %s\n", inet_ntoa(conf.dev_addr));

  if (pcap_compile(handle, &fp, "tcp", 1, net) == -1) {
    fprintf(stderr, "Couldn't parse filter...");
    return 1;
  }

  if (pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter: %s\n", pcap_geterr(handle));
    return 1;
  }

  pcap_loop(handle, -1, handle_packet, (u_char *)&conf);

  return 0;
}
