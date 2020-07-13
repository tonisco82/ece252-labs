#include <curl/multi.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

static size_t cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}

static void curl_multi_init(CURLM *cm, char *url)
{
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt(eh, CURLOPT_URL, url);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, url);
  curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
  curl_multi_add_handle(cm, eh);
}