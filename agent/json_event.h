#ifndef JSON_EVENT_H_
#define JSON_EVENT_H_

int append_event_json_str(char *buffer, const u_int buf_len, int offset, const char *key, const char *value);
int append_event_json_int(char *buffer, const u_int buf_len, int offset, const char *key, const u_int value);
int setup_event_json_for_append(char *buffer, const u_int buf_len, int offset); 
int close_event_json(char *buffer, const u_int buf_len, int offset);

#endif
