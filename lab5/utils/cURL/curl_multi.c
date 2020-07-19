#include <curl/multi.h>
#include <stdio.h>
#include <stdlib.h>
#include "curl_xml_fns.c"

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */


/**
 * @brief create a curl easy handle and set the options.
 * @param RECV_BUF *ptr points to user data needed by the curl write call back function
 * @param const char *url is the target url to fetch resoruce
 * @return a valid CURL * handle upon sucess; NULL otherwise
 * Note: the caller is responsbile for cleaning the returned curl handle
 */
// CURL *multi_single_handle_init(const char *url)
// {
//     if (url == NULL) {
//         return NULL;
//     }

//     CURL *curl_handle = NULL;

//     /* init a curl session */
//     curl_handle = curl_easy_init();

//     if (curl_handle == NULL) {
//         fprintf(stderr, "curl_easy_init: returned NULL\n");
//         return NULL;
//     }

//     RECV_BUF *recv_buf = (RECV_BUF *) malloc(sizeof(RECV_BUF));
//     if ( recv_buf_init(recv_buf, BUF_SIZE) != 0 ) {
//         free(recv_buf);
//         return NULL;
//     }

//     /* specify URL to get */
//     curl_easy_setopt(curl_handle, CURLOPT_URL, url);
//     /* register write call back function to process received data */
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
//     /* user defined data structure passed to the call back function */
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)recv_buf);
//     /* register header call back function to process received header data */
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
//     /* user defined data structure passed to the call back function */
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)recv_buf);
//     /* some servers requires a user-agent field */
//     curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");
//     /* follow HTTP 3XX redirects */
//     curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
//     /* continue to send authentication credentials when following locations */
//     curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
//     /* max numbre of redirects to follow sets to 5 */
//     curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
//     /* supports all built-in encodings */ 
//     curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");
//     /* Enable the cookie engine without reading any initial cookies */
//     curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
//     /* allow whatever auth the proxy speaks */
//     curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
//     /* allow whatever auth the server speaks */
//     curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

//     return curl_handle;
// }

int curl_multi_init_easy(CURLM *cm, const char *url)
{
  RECV_BUF *recv_buf = malloc(sizeof(RECV_BUF));
  CURL *curl_handle = easy_handle_init(recv_buf, url);

  if ( curl_handle == NULL ) {
    fprintf(stderr, "Curl initialization failed. Exiting...\n");
    return 1;
  }

  curl_easy_setopt(curl_handle, CURLOPT_PRIVATE, recv_buf);

  /* Add to Multi */
  curl_multi_add_handle(cm, curl_handle);

  return 0;
}