#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pcap_conf.h"
#include "http_post.h"

int init_batch(char *batch_buf, const u_int buf_len, struct pcap_conf *conf)
{
  return snprintf(batch_buf, buf_len, "{\"dev_ip\":\"%s\", \"data\": [", inet_ntoa(conf->dev_addr));
}

int check_batch_fit(const u_int batch_buf_len, int offset, const u_int event_buf_len)
{
  return ((int)batch_buf_len - offset - (int)event_buf_len) > 3; /* leave enough room for ]}\0 */
}

void close_batch(char *batch_buf, const u_int buf_len, int offset)
{
  snprintf(batch_buf + offset, buf_len - offset, "]}");
}

int send_batch(char *batch_buf, struct pcap_conf *conf)
{
  return curl_post(conf->url, batch_buf);
}

int append_batch_event(char *batch_buf, const u_int batch_buf_len, int offset, char *event_buf, const u_int event_buf_len, struct pcap_conf *conf)
{
  if (offset == 0) {
    offset = init_batch(batch_buf, batch_buf_len, conf);
    offset += snprintf(batch_buf + offset, batch_buf_len - offset, "%s", event_buf);
  } else if (!check_batch_fit(batch_buf_len, offset, event_buf_len)) {
    close_batch(batch_buf, batch_buf_len, offset);
    send_batch(batch_buf, conf);

    offset = init_batch(batch_buf, batch_buf_len, conf);
    offset += snprintf(batch_buf + offset, batch_buf_len - offset, "%s", event_buf);
  } else {
    offset += snprintf(batch_buf + offset, batch_buf_len - offset, ",%s", event_buf);
  }

  return offset;
}
