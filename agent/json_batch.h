#ifndef JSON_BATCH_H_
#define JSON_BATCH_H_

int init_batch(char * const batch_buf, const u_int buf_len, const struct pcap_conf * const conf);
int check_batch_fit(const u_int batch_buf_len, int offset, const u_int event_buf_len);

void close_batch(char * const batch_buf, const u_int buf_len, int offset);
int send_batch(char * const batch_buf, const struct pcap_conf * const conf);

int append_batch_event(char * const batch_buf, const u_int batch_buf_len, int offset, char * const event_buf, const u_int event_buf_len, const struct pcap_conf * const conf);

#endif
