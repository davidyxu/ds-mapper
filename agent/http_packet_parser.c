#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int is_http_res(const char * const payload, const int len)
{
  if (len >= 12 && (strncmp("HTTP/", payload, 5) == 0)) {
    char http_code[5];
    memcpy(http_code, &payload[9], 4);
    http_code[4] = '\0';

    return atoi(http_code);
  }
  return 0;
}

int is_http_req(const char * const payload, const int len)
{
  if (strncmp("GET",     payload, 3) &&
      strncmp("PUT",     payload, 3) &&
      strncmp("POST",    payload, 4) &&
      strncmp("HEAD",    payload, 4) &&
      strncmp("TRACE",   payload, 5) &&
      strncmp("DELETE",  payload, 6) &&
      strncmp("CONNECT", payload, 7) &&
      strncmp("OPTIONS", payload, 7)) {
    return 0;
  } else {
    return 1;
  }
}

int is_http_packet(const char * const payload, const int len)
{
  return (is_http_res(payload, len) || is_http_req(payload, len));
}

void get_ascii_payload(char * const buffer, const char *payload, const int len)
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
