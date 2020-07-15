/** 
 * @brief Web Crawler to search the web for PNG images.
 * @authors Braden Bakker and Devon Miller-Junk
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/multi.h>
#include <search.h>
#include <time.h> // For program run time

//Included Utility Code
#include "./utils/linkedList.c"
#include "./utils/cURL/curl_multi.c"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "

// Default Values for Inputs and File Names
#define OUTPUT_FILE "png_urls.txt"
#define DEFAULT_THREAD_NUM 1
#define DEFAULT_PNG_NUM 50
#define DEFAULT_LOG_FILE ""

#define DEFAULT_STRING_SZ 250 // Size of the Default String

#define MAX_HTABLE_SZ 500 // Size of the HashTable

// Web Crawler Input
typedef struct web_crawler_input {
    Node_t **png_urls; // The urls of valid PNGs
    Node_t **to_visit; // A list (stack) of the next URLs to Visit
    Node_t **log; // A log of every URL visited in order
    int numpicture; // Total Number of Pictures Left to Acquire
    int total_connections; // Maximum number of concurrent connections at once
} web_crawler_input_t;

// Main Function for Retrieving URL information
int retrieve_urls(web_crawler_input_t crawler_in);


/**
 * Main Function to create and run the functions
 */
int main(int argc, char *argv[]) {
    //TODO: need to rework input decoding to work when the seed url isn't the last input.

    /** @initial_section: Input Validation and Setup **/

        // Start Timer and Variables
    struct timeval program_start, program_end;
    if (gettimeofday(&program_start, NULL) != 0) {
        perror("gettimeofday");
        return -1;
    }

        //Input Variables
    char* seed_url = (char *) malloc(DEFAULT_STRING_SZ * sizeof(char));
    strcpy(seed_url, (argc > 1 && argc % 2 == 0) ? argv[argc-1] : SEED_URL);

    int total_connections = DEFAULT_THREAD_NUM;

    int numpicture = DEFAULT_PNG_NUM;

    char* logfile = (char *) malloc(DEFAULT_STRING_SZ * sizeof(char));
    strcpy(logfile, DEFAULT_LOG_FILE);


        // Fill Inputs
	int c;
    while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
        switch (c) {
        case 't':
            total_connections = strtoul(optarg, NULL, 10);
            if (total_connections < 1){
            	perror("number of threads must be 1 or more -- 't'\n");
                return -1;
            }
            break;
        case 'm':
            numpicture = strtoul(optarg, NULL, 10);
            if (numpicture < 1) {
                perror("number of picture must be 1 or more -- 'n'\n");
                return -1;
            }
            break;
        case 'v':
            strcpy(logfile, optarg);
            break;
        default:
            perror("Usage example: ./findpng2 -t 10 -m 50 -v log.txt\n");
            free(logfile);
            free(seed_url);
            return -1;
        }
    }

        // Program Variables
    Node_t *png_urls = NULL; // The resulting png urls (of valid PNGs)
    Node_t *to_visit = NULL; // A list (stack) of the next URLs to Visit. After pop need to ensure the URL has not been checked before (but also check before putting in to save memory)
    Node_t *log = NULL; // A log of every URL visited in order
    struct hsearch_data *visited_urls = calloc(1, sizeof(struct hsearch_data));
    if(hcreate_r(MAX_HTABLE_SZ, visited_urls) == 0){
        perror("Error: insufficient space to create hashtable\n");
        free(logfile);
        free(seed_url);
        free(visited_urls);
        return -1;
    }

    /** @main_section: Start Main Part of the Program **/

        // Push Initial Seed URL
    push(&to_visit, seed_url);

        //Main Function Call
    web_crawler_input_t crawler_in;
    crawler_in.png_urls = &png_urls;
    crawler_in.to_visit = &to_visit;
    crawler_in.log = &log;
    crawler_in.numpicture = numpicture;
    crawler_in.total_connections = total_connections;
    if(retrieve_urls(crawler_in) != 0){
        perror("Error: retrieve_urls returned error\n");
        free(logfile);
        free(seed_url);
        free(visited_urls);
        freeMemory(png_urls);
        return -1;
    }

    /** @output_section: Output Timing Result **/

        // Output to logfile if applicable
    if(strcmp(logfile, DEFAULT_LOG_FILE) != 0) linkedlist_to_file(log, logfile);

        // Output valid PNG urls to file
    linkedlist_to_file(png_urls, OUTPUT_FILE);

        // Print Timing
    gettimeofday(&program_end, NULL);
    printf("findpng2 execution time: %.6lf seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));

    /** @final_section: Cleanup **/

        //Free Memory
    free(logfile);
    freeMemory(png_urls);
    freeMemory(to_visit);
    freeMemory(log);

    return 0;
}

/**
 * @brief: Retrieve the next URL in the list that is not in the hash table.
 * @params:
 * head_node: linked list of urls
 * htable: hash table in question.
 * @return: NULL if no url matches, or pointer to a matching url.
 */
char *get_next_valid_url(Node_t **head_node, struct hsearch_data *htable){
    // Initial Setup
    ENTRY url_entry, *ret_entry;
    url_entry.data = NULL;
    
    // Check for new URL
    while(*head_node != NULL){
        // Pop url
        url_entry.key = pop(head_node);
        if(url_entry.key == NULL) return NULL;

        //Search in hashtable
        hsearch_r(url_entry, FIND, &ret_entry, htable);
        if(ret_entry == NULL){
            // Add to hashtable
            hsearch_r(url_entry, ENTER, &ret_entry, htable);
            if(ret_entry == NULL){ //If hashtable is full
                perror("Error: Hashtable is full\n");
                free(url_entry.key);
                return NULL;
            }
            return url_entry.key;
        }

        // Free
        free(url_entry.key);
    }
    return NULL;
}


/**
 * @brief: Retrieve URLs in a crawler, and log the PNG images retrieved.
 * @params:
 * crawler_in: input data
 * @return:
 * 0: sucess
 * other: error
 */
int retrieve_urls(web_crawler_input_t crawler_in){

    /** Initial Variable Setup **/
    int still_running = 0, res, msgs_left = 0;
    char *url = NULL;

    // Setup Hashtable
    struct hsearch_data *visited_urls = calloc(1, sizeof(struct hsearch_data));
    if(hcreate_r(MAX_HTABLE_SZ, visited_urls) == 0){
        perror("Error: insufficient space to create hashtable\n");
        free(visited_urls);
        return -1;
    }

        // Initialize cURL
    curl_global_init(CURL_GLOBAL_ALL);
    CURLM *cm = curl_multi_init();
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, crawler_in.total_connections);
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
    CURL *curl_handle = NULL;
    CURLMsg *msg = NULL;

    curl_multi_init_easy(cm, get_next_valid_url(crawler_in.to_visit, visited_urls));

    /** Main Loop **/
    while(crawler_in.numpicture > 0){

        // Perform Requests
        res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, NULL);
        if(res != CURLM_OK) {
            fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
            return EXIT_FAILURE;
        }
        curl_multi_perform(cm, &still_running);

        // Process Results Continuously
        while (msg = curl_multi_info_read(cm, &msgs_left)) {
            if (msg->msg == CURLMSG_DONE) {
                curl_handle = msg->easy_handle;

                curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &mem); //TODO: replace with recv_buffer data pointer. Need to initialize the buffer.

                if(msg->data.result == CURLE_OK){
                    // Process Data
                    multi_process_data(curl_handle, &recv_buf, crawler_in.to_visit, crawler_in.png_urls, visited_urls); // TODO: hook up to right recv_buffer pointer
                }

                curl_multi_remove_handle(cm, curl_handle);
                curl_easy_cleanup(curl_handle);
            }
            else {
                fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
            }
        }
    }
        // cURL cleanup
    curl_multi_cleanup(cm);

        // cURL cleanup
    curl_global_cleanup();

        //Free Memory
    hdestroy_r(visited_urls);
    free(visited_urls);

    return 0;
}