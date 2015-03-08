#include <stdio.h>
#include <curl/curl.h>

int curl_post(const char *url, char *post_data)
{
  long http_code = 0;

  CURL *curl;
  CURLcode res;

  struct curl_slist *headers=NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl = curl_easy_init();

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      return -1;
    }

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();

  return http_code;
}
