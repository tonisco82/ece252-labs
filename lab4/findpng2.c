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
#include <curl/curl.h>
#include <search.h>
#include <time.h> // For program run time
#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>

//Included Utility Code
#include "./utils/linkedList.c"
#include "./utils/cURL/curl_xml_fns.c"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "

// Default Values for Inputs and File Names
#define OUTPUT_FILE "png_urls.txt"
#define DEFAULT_THREAD_NUM 1
#define DEFAULT_PNG_NUM 50
#define DEFAULT_LOG_FILE ""

#define DEFAULT_STRING_SZ 250 // Size of the Default String

#define MAX_HTABLE_SZ 500 // Size of the HashTable

#define SEM_SHARE 0 // Only Share with Other Threads

/**
 * Data Type For Input Parameters into the Thread Function
 */
typedef struct web_crawler_input {
    Node_t **png_urls; // The urls of valid PNGs.
    Node_t **to_visit; // A list (stack) of the next URLs to Visit.
    Node_t **log; // A log of every URL visited in order
    struct hsearch_data *visited_urls; // Hash Table for all URLs
    int *numpicture; // Total Number of Pictures Left to Acquire
    int *num_waiting; // The number of threads currently waiting for a URL to process
    pthread_cond_t *to_visit_threshold_cv; // Conditional Variable for Signalling
    pthread_mutex_t *to_visit_m; // Access Control for to_visit
    pthread_mutex_t *png_urls_m; // Access Control for png_urls
    pthread_mutex_t *log_m; // Access Control for log
    pthread_rwlock_t *hashtable_m; // Access Control for hashtable of all urls.
    int total_threads;
} web_crawler_input_t;

/**
 * Function To Crawl the Web and Find N PNG files. Is thread safe.
 */
void *web_crawler(void *arg);

/**
 * @brief: Function To Retrieve the information from the specified URL. Thread-safe
 * See more below.
 */
int fetch_information(Node_t **to_visit, Node_t** png_urls, char* url);

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

    int numthreads = DEFAULT_THREAD_NUM;

    int numpicture = DEFAULT_PNG_NUM;

    char* logfile = (char *) malloc(DEFAULT_STRING_SZ * sizeof(char));
    strcpy(logfile, DEFAULT_LOG_FILE);


        // Fill Inputs
	int c;
    while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
        switch (c) {
        case 't':
            numthreads = strtoul(optarg, NULL, 10);
            if (numthreads < 1){
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

        // Semaphores, Mutexes, and Thread Control
    pthread_cond_t to_visit_threshold_cv; // Conditional Variable for Signalling
    pthread_mutex_t to_visit_m; // Access Control for to_visit
    pthread_mutex_t png_urls_m; // Access Control for png_urls
    pthread_mutex_t log_m; // Access Control for log
    pthread_rwlock_t hashtable_m; // Access Control for hashtable of all urls.
    if (pthread_cond_init(&to_visit_threshold_cv, NULL) != 0 || pthread_mutex_init(&to_visit_m, NULL) != 0 || pthread_mutex_init(&png_urls_m, NULL) != 0 || pthread_mutex_init(&log_m, NULL) != 0 || pthread_rwlock_init(&hashtable_m, NULL) != 0) {
        perror("sem_init(sem)\n");
        free(logfile);
        free(seed_url);
        return -1;
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

    int num_waiting = 0; // Track the number of threads waiting for a URL to process

    web_crawler_input_t crawler_params;
    crawler_params.png_urls = &png_urls;
    crawler_params.to_visit = &to_visit;
    crawler_params.log = &log;
    crawler_params.visited_urls = visited_urls;
    crawler_params.numpicture = &numpicture;
    crawler_params.num_waiting = &num_waiting;
    crawler_params.to_visit_threshold_cv = &to_visit_threshold_cv;
    crawler_params.to_visit_m = &to_visit_m;
    crawler_params.png_urls_m = &png_urls_m;
    crawler_params.log_m = &log_m;
    crawler_params.hashtable_m = &hashtable_m;
    crawler_params.total_threads = numthreads;

    pthread_t pthread_ids[numthreads];

    /** @main_section: Start Main Part of the Program **/

        // Push Initial Seed URL
    push(&to_visit, seed_url);

        // Initialize cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
        // Create Threads
    for(int i=0; i<numthreads; i++){
        pthread_create(pthread_ids + i, NULL, web_crawler, (void *)(&crawler_params));
    }

        // Join Threads
    for(int i=0; i<numthreads; i++){
        pthread_join(pthread_ids[i], NULL);
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

        //Destroy Semaphores and mutexes
    pthread_cond_destroy(&to_visit_threshold_cv);
    pthread_mutex_destroy(&to_visit_m);
    pthread_mutex_destroy(&png_urls_m);
    pthread_mutex_destroy(&log_m);
    pthread_rwlock_destroy(&hashtable_m);

        //Free Memory
    hdestroy_r(visited_urls);
    free(visited_urls);
    free(logfile);
    freeMemory(png_urls);
    freeMemory(to_visit);
    freeMemory(log);

    curl_global_cleanup();

    return 0;
}


/**
 * @brief: Function To Crawl the Web and Find N PNG files. Is thread safe.
 */
void *web_crawler(void *arg){
    /** @initial_setup: decode input and set up variables **/
        // Decode Input
	web_crawler_input_t *input_data = arg;
        // Variables
    int findMorePNGs = 1; // Become 0 if desired number of pngs is reached
    char *url, *url_cpy;
    ENTRY url_entry, *ret_entry;
    url_entry.data = NULL;
    int visited = 0;
    Node_t *png_urls = NULL;
    Node_t *to_visit = NULL;
    int inserted = 0;


    while(findMorePNGs){
        /** @critical_section: Waiting For A URL, otherwise take a URL **/
        pthread_mutex_lock(input_data->to_visit_m);
            // If no URLs available, wait for one
        if((*(input_data->to_visit)) == NULL){
            (*(input_data->num_waiting))++;
            if(*(input_data->num_waiting) < input_data->total_threads){
                pthread_cond_wait(input_data->to_visit_threshold_cv, input_data->to_visit_m);
            }
            if(*(input_data->num_waiting) == input_data->total_threads && (*(input_data->to_visit)) == NULL){
                pthread_cond_broadcast(input_data->to_visit_threshold_cv);
                findMorePNGs = 0;
                pthread_mutex_unlock(input_data->to_visit_m);
                continue;
            }
            (*(input_data->num_waiting))--;
        }
        url = pop(input_data->to_visit);
        pthread_mutex_unlock(input_data->to_visit_m);
        /** @end_critical_section: **/

        if(url != NULL){
            url_entry.key = url;

            /** @critical_section: see if the url is in the hashtable **/
            pthread_rwlock_wrlock(input_data->hashtable_m);
            hsearch_r(url_entry, FIND, &ret_entry, input_data->visited_urls);
            if(ret_entry == NULL){
                hsearch_r(url_entry, ENTER, &ret_entry, input_data->visited_urls);
                visited = 1;
                if(ret_entry == NULL){
                    perror("Error: Hashtable is full\n");
                    pthread_rwlock_unlock(input_data->hashtable_m);
                    free(url);
                    abort();
                }
            }
            pthread_rwlock_unlock(input_data->hashtable_m);
            /** @end_critical_section: **/
        }

        // Perform Request if URL has not been visited
        if(visited){
            /** @critical_section: insert into log **/
            pthread_mutex_lock(input_data->log_m);
            push(input_data->log, url);
            pthread_mutex_unlock(input_data->log_m);
            /** @end_critical_section: **/

            //Perform cURL Request and Retrieve Data
            if(fetch_information(&to_visit, &png_urls, url) == 0){
                if(png_urls != NULL){
                    while(png_urls != NULL){
                        inserted = 0;
                        // Make Copy of String
                        url_cpy = pop(&png_urls);
                        /** @critical_section: insert into png_urls **/
                        pthread_mutex_lock(input_data->png_urls_m);
                        if(*(input_data->numpicture) > 0){
                            inserted = 1;
                            (*(input_data->numpicture))--;
                            push(input_data->png_urls, url_cpy);
                        }
                        if(*(input_data->numpicture) <= 0){
                            findMorePNGs = 0;
                            freeMemory(png_urls);
                            png_urls = NULL;
                        }
                        pthread_mutex_unlock(input_data->png_urls_m);
                        /** @end_critical_section: **/

                        if(inserted == 0) free(url_cpy);
                    }
                }else if(to_visit != NULL){
                    /** @critical_section: add URLs to visit (after checking if already searched) **/
                    pthread_rwlock_rdlock(input_data->hashtable_m); // only a read lock since just reading
                    while(to_visit != NULL){
                        url_entry.key = pop(&to_visit);
                        hsearch_r(url_entry, FIND, &ret_entry, input_data->visited_urls);
                        if(ret_entry == NULL && url_entry.key != NULL){
                            /** @critical_section: inner critical section for pushing to the linked list and signalling push **/
                            pthread_mutex_lock(input_data->to_visit_m);
                            push(input_data->to_visit, url_entry.key);
                            pthread_cond_signal(input_data->to_visit_threshold_cv);
                            pthread_mutex_unlock(input_data->to_visit_m);
                            /** @end_critical_section: **/
                        }else{
                            free(url_entry.key);
                        }
                    }
                    pthread_rwlock_unlock(input_data->hashtable_m);
                    /** @end_critical_section: **/
                }
            }
        }else{
            free(url);
        }

        if(findMorePNGs){
            /** @critical_section: check to see if end condition met **/
            pthread_mutex_lock(input_data->png_urls_m);
            if(*(input_data->numpicture) <= 0){
                findMorePNGs = 0;
            }
            pthread_mutex_unlock(input_data->png_urls_m);
            /** @end_critical_section: **/
        }

        visited = 0;
    }

    /** @critical_section: Ensure Other Threads Exit If PNG Limit Reached **/
    pthread_mutex_lock(input_data->to_visit_m);
    (*(input_data->num_waiting)) = input_data->total_threads;
    pthread_cond_broadcast(input_data->to_visit_threshold_cv);
    pthread_mutex_unlock(input_data->to_visit_m);
    /** @end_critical_section: **/

    return NULL;
}

/**
 * @brief: Function To Retrieve the information from the specified URL. Thread-safe
 * @params:
 * to_visit: pointer to an empty linked list. Use push function to add urls to visit into the list.
 * png_urls: pointer to an empty linked list. Use push function to add png urls into the list.
 * url: pointer to a url. Perform cURL on this url.
 * @return:
 * 0: success
 * 1: error
 * @note: for the linked lists, declare the data values on the heap. Will be deallocated by caller.
 * @note: if to_visit or png_urls are empty, they should be left as passed in. Only call push when adding a value.
 */
int fetch_information(Node_t **to_visit, Node_t** png_urls, char* url){
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
    curl_handle = easy_handle_init(&recv_buf, url);

    if ( curl_handle == NULL ) {
        //fprintf(stderr, "Curl initialization failed. Exiting...\n");
        curl_global_cleanup();
        return 1;
    }
    /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        cleanup(curl_handle, &recv_buf);
        return 1;
    } else {
        //For Logging and Debugging if needed
        //printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf.size, recv_buf.buf, recv_buf.seq);
    }

    /* process the download data */
    if (process_data(curl_handle, &recv_buf, to_visit, png_urls) != 0){
        cleanup(curl_handle, &recv_buf);
        return 1;
    }

    /* cleaning up */
    cleanup(curl_handle, &recv_buf);
    return 0;
}