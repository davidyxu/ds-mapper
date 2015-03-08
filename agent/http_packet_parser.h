#ifndef HTTP_PACKET_PARSER_H_
#define HTTP_PACKET_PARSER_H_
extern enum HttpMethod;
const int get_http_code(const u_char *payload, int len);
enum HttpMethod get_http_method(const u_char *payload, int len);

#endif
