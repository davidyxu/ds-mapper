#ifndef JSON_BATCH_H_
#define JSON_BATCH_H_

int init_batch(char *batch_buf, const u_int buf_len, struct pcap_conf *conf);
int check_batch_fit(const u_int batch_buf_len, int offset, const u_int event_buf_len);

void close_batch(char *batch_buf, const u_int buf_len, int offset);
int send_batch(char *batch_buf);

int append_batch_event(char *batch_buf, const u_int batch_buf_len, int offset, char *event_buf, const u_int event_buf_len);

#endif
