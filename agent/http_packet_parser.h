#ifndef HTTP_PACKET_PARSER_H_
#define HTTP_PACKET_PARSER_H_

const int is_http_packet(const char * const payload, const int len);
const int is_http_req(const char * const payload, const int len);
const int is_http_res(const char * const payload, const int len);
void get_ascii_payload(char * const buffer, const char *payload, const int len);

#endif
