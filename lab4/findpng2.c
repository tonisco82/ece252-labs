/** 
 * @brief Web Crawler to search the web for PNG images.
 * @authors Braden Bakker and Devon Miller-Junk
 */ 

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
// #include <libxml/HTMLparser.h>
// #include <libxml/parser.h>
// #include <libxml/xpath.h>
// #include <libxml/uri.h>

//Included Utility Code
#include "./utils/linkedList.c"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "

// Default Values for Inputs and File Names
#define OUTPUT_FILE "png_urls.txt"
#define DEFAULT_THREAD_NUM 1
#define DEFAULT_PNG_NUM 50
#define DEFAULT_LOG_FILE ""

#define DEFAULT_STRING_SZ 200 // Size of the Default String

#define MAX_HTABLE_SZ 500 // Size of the HashTable

/**
 * Data Type For entries into the Hash Table
 */
typedef struct Hash_URL_Entry {
    char *url;
    char state;
    /**
    * @state: A classification system for the url. Indicated what the data is.
    * All URLs in the hash table should have already be visited and processed.
    * @values:
    * 0: currently being processed by another thread.
    * 1: valid PNG
    * 2: valid URLs to crawl (link to more URLs)
    * 3: invalid URL
    */
} hash_url_entry_t;

/**
 * Data Type For Input Parameters into the Thread Function
 */
typedef struct web_crawler_input {
    Node_t **png_urls; // The urls of valid PNGs.
    Node_t **to_visit; // A list (stack) of the next URLs to Visit.
    Node_t **log; // A log of every URL visited in order
    struct hsearch_data *visited_urls; // Hash Table for all URLs
    int *numpicture; // Total Number of Pictures Left to Acquire
} web_crawler_input_t;

/**
 * Function To Crawl the Web and Find N PNG files. Is thread safe.
 */
void *web_crawler(void *arg);

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
            	printf("%s: number of threads must be 1 or more -- 't'\n", argv[0]);
                return -1;
            }
            break;
        case 'm':
            numpicture = strtoul(optarg, NULL, 10);
            if (numpicture < 1) {
                printf("%s: number of picture must be 1 or more -- 'n'\n", argv[0]);
                return -1;
            }
            break;
        case 'v':
            strcpy(logfile, optarg);
            break;
        default:
            printf("Usage example: ./findpng -t 10 -m 50 -v png_urls.txt\n");
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
        printf("Error: insufficient space to create hashtable\n");
        free(logfile);
        free(seed_url);
        free(visited_urls);
        return -1;
    }

    web_crawler_input_t crawler_params;
    crawler_params.png_urls = &png_urls;
    crawler_params.to_visit = &to_visit;
    crawler_params.log = &log;
    crawler_params.visited_urls = visited_urls;
    crawler_params.numpicture = &numpicture;

    pthread_t pthread_ids[numthreads];

    /** @main_section: Start Main Part of the Program **/

        // Push Initial Seed URL
    push(&to_visit, seed_url);

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
    
    hdestroy_r(visited_urls);
    free(visited_urls);
    free(logfile);
    //free(seed_url); // Should be freed when removed from the list.
    freeMemory(png_urls);
    freeMemory(to_visit);
    freeMemory(log);

    return 0;
}


/**
 * @brief: Function To Crawl the Web and Find N PNG files. Is thread safe.
 * @steps:
 * 1. Check to see if there are URLs in the to_visit stack
 *      a. If there is one, take it
 *      b. If not, check to see if any URLs are being processed.
 *          i. If there are, wait for it to finish.
 *          ii. If not, exit the program
 * 2. Check to see if the URL has been processed. (check HashTable)
 *      a. If it has, discard it. Go to step 1.
 *      b. If it hasn't, add to hashtable as searching. Perform cURL request.
 * 3. Once cURL is finished:
 *      a. Update value in hashtable
 *      b. If PNG, add to png_urls if numpngs still positive.
 *      c. Add the list of linked URLs to the to_visit
 */
void *web_crawler(void *arg){
    /** @initial_setup: decode input and set up variables **/
	// web_crawler_input_t *input_data = arg;

    // int findMorePNGs = 1; // Become 0 if numpngs is 

    // while(findMorePNGs){

    // }

    printf("Testing123\n");

    return NULL;
}

//Done when:
// 1. There are no more items in the to_visit list && there are no more URLs being processed
// or 
// 2. The max number of PNGs has been found