#ifndef JSON_EVENT_H_
#define JSON_EVENT_H_

int append_event_json_str(char * const buffer, const u_int buf_len, int offset, const char * const key, const char * const value);
int append_event_json_int(char * const buffer, const u_int buf_len, int offset, const char * const key, const u_int value);
int setup_event_json_for_append(char * const buffer, const u_int buf_len, int offset);
int close_event_json(char * const buffer, const u_int buf_len, int offset);

#endif
