#include <sys/types.h>
#include <string.h>

enum HttpMethod
{
  NIL,
  GET,
  PUT,
  POST,
  HEAD,
  TRACE,
  DELETE,
  CONNECT,
  OPTIONS
};

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

enum HttpMethod get_http_method(const u_char *payload, int len)
{
  // check if length is greater than 7 for all
  // full tcp payload size will be larger than this even for GET
  if (len >= 7) {
    if (!strncmp("GET", payload, 3)) {
      return GET;
    } else if (!strncmp("PUT", payload, 3)) {
      return PUT;
    } else if (!strncmp("POST", payload, 4)) {
      return POST;
    } else if (!strncmp("HEAD", payload, 4)) {
      return HEAD;
    } else if (!strncmp("TRACE", payload, 5)) {
      return TRACE;
    } else if (!strncmp("DELETE", payload, 6)) {
      return DELETE;
    } else if (!strncmp("CONNECT", payload, 7)) {
      return CONNECT;
    } else if (!strncmp("OPTIONS", payload, 7)) {
      return OPTIONS;
    }
  }

  return NIL;
}
