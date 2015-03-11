#include <sys/types.h>
#include <stdio.h>

int append_event_json_str(char *buffer, const u_int buf_len, int offset, const char *key, const char *value)
{
  offset = setup_event_json_for_append(buffer, buf_len, offset);
  offset += snprintf(buffer + offset, buf_len - offset, "\"%s\":\"%s\"", key, value);

  return offset;
}

int append_event_json_int(char *buffer, const u_int buf_len, int offset, const char *key, const u_int value)
{
  offset = setup_event_json_for_append(buffer, buf_len, offset);
  offset += snprintf(buffer +offset, buf_len - offset, "\"%s\":%d", key, value);

  return offset;
}

int setup_event_json_for_append(char *buffer, const u_int buf_len, int offset)
{
  if (offset == 0)
    offset += snprintf(buffer, buf_len, "{");
  else
    offset += snprintf(buffer + offset, buf_len - offset, ",");

  return offset;
}

int close_event_json(char *buffer, const u_int buf_len, int offset)
{
  if (offset == 0)
    offset += snprintf(buffer, buf_len, "{}");
  else
    offset += snprintf(buffer + offset, buf_len - offset, "}");

  return offset;
}
