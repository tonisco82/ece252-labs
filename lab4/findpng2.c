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
    sem_t *urls_to_visit_sem; // Number of Items in the to_visit list
    pthread_mutex_t *to_visit_m; // Access Control for to_visit
    pthread_mutex_t *png_urls_m; // Access Control for png_urls
    pthread_mutex_t *log_m; // Access Control for log
    pthread_rwlock_t *hashtable_m; // Access Control for hashtable of all urls.
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

        // Semaphores, Mutexes, and Thread Control
    sem_t urls_to_visit_sem; // Number of Items in the to_visit list
    pthread_mutex_t to_visit_m; // Access Control for to_visit
    pthread_mutex_t png_urls_m; // Access Control for png_urls
    pthread_mutex_t log_m; // Access Control for log
    pthread_rwlock_t hashtable_m; // Access Control for hashtable of all urls.
    if ( sem_init(&urls_to_visit_sem, SEM_SHARE, 1) != 0 || pthread_mutex_init(&to_visit_m, NULL) != 0 || pthread_mutex_init(&png_urls_m, NULL) != 0 || pthread_mutex_init(&log_m, NULL) != 0 || pthread_wrlock_init(&hashtable_m, NULL) != 0) {
        printf("sem_init(sem)\n");
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
    crawler_params.urls_to_visit_sem = &urls_to_visit_sem;
    crawler_params.to_visit_m = &to_visit_m;
    crawler_params.png_urls_m = &png_urls_m;
    crawler_params.log_m = log_m;
    crawler_params.hashtable_m = &hashtable_m;

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

        //Destroy Semaphores and mutexes
    sem_destroy(&urls_to_visit_sem);
    pthread_mutex_destroy(&to_visit_m);
    pthread_mutex_destroy(&png_urls_m);
    pthread_mutex_destroy(&log_m);
    pthread_rwlock_destroy(&hashtable_m);

        //Free Memory
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
 */
void *web_crawler(void *arg){
    /** @initial_setup: decode input and set up variables **/
        // Decode Input
	web_crawler_input_t *input_data = arg;
        // Variables
    int findMorePNGs = 1; // Become 0 if numpngs is
    char *url, *url_cpy;
    ENTRY url_entry, *ret_entry;
    url_entry.data = NULL;
    int visited = 0;

    while(findMorePNGs){
        // Wait for a PNG to enter the 
        sem_wait(input_data->urls_to_visit_sem);

        /** @critical_section: take a url **/
        pthread_mutex_lock(input_data->to_visit_m);
        url = pop(input_data->to_visit);
        pthread_mutex_unlock(input_data->to_visit_m);
        /** @end_critical_section: **/

        url_entry.key = url;

        /** @critical_section: see if the url is in the  **/
        pthread_rwlock_wrlock(input_data->hashtable_m);
        hsearch_r(url_entry, FIND, &ret_entry, input_data->visited_urls);
        if(ret_entry == NULL){
            hsearch_r(url_entry, ENTER, &ret_entry, input_data->visited_urls);
            visited = 1;
            if(ret_entry == NULL){
                printf("Error: Hashtable is full\n");
                pthread_rwlock_unlock(input_data->hashtable_m);
                abort();
            }
        }
        pthread_rwlock_unlock(input_data->hashtable_m);
        /** @end_critical_section: **/

        // Perform Request if URL has not been visited
        if(!visited){
            //Perform cURL Request and Retrieve Data
            //TODO: Insert cURL Call Function Here.

            /** @critical_section: insert into log **/
            pthread_mutex_lock(input_data->log_m);
            push(input_data->log, url);
            pthread_mutex_unlock(input_data->log_m);
            /** @end_critical_section: **/

            if(urlToPNGs){ //TODO: replace with png detection from cURL request.
                for(int i=0;i<URL_TO_PNG.length;i++){
                    // Make Copy of String
                    url_cpy = malloc((1 + strlen(URL_TO_USE)) * (sizeof(char)));
                    strcpy(url_cpy, URL_TO_USE);

                    /** @critical_section: insert into png_urls **/
                    pthread_mutex_lock(input_data->png_urls_m);
                    if(input_data->numpicture > 0){
                        input_data->numpicture--;
                        push(input_data->png_urls, url_cpy);
                    }else{
                        findMorePNGs = 0;
                        i = URL_TO_PNG.length;
                    }
                    pthread_mutex_unlock(input_data->png_urls_m);
                    /** @end_critical_section: **/
                }
            }

            if(findMorePNGs && urlToVisit){ //TODO: replace with future URLs to look thru if exists
                /** @critical_section: add URLs to visit (after checking if already searched) **/
                pthread_rwlock_rdlock(input_data->hashtable_m); // only a read lock since just reading
                for(int i=0;i<ToVisit.length;i++){
                    url_entry.key = INSERT_URL_HERE;
                    hsearch_r(url_entry, FIND, &ret_entry, input_data->visited_urls);
                    if(ret_entry == NULL){
                        /** @critical_section: inner critical section for pushing to the linked list **/
                        pthread_mutex_lock(input_data->to_visit_m);
                        push(input_data->to_visit, url_entry.key);
                        pthread_mutex_unlock(input_data->to_visit_m);
                        /** @end_critical_section: **/

                        // Post to indicate another URL on the to_visit list
                        sem_post(input_data->urls_to_visit_sem);
                    }
                }
                pthread_rwlock_unlock(input_data->hashtable_m);
                /** @end_critical_section: **/
            }
        }
        
        
        if(findMorePNGs){
            /** @critical_section: check to see if end condition met **/
            pthread_mutex_lock(input_data->png_urls_m);
            if(input_data->numpicture <= 0){
                findMorePNGs = 0;
            }
            pthread_mutex_unlock(input_data->png_urls_m);
            /** @end_critical_section: **/
        }

        visited = 0;
    }

    return NULL;
}

//Done when:
// 1. There are no more items in the to_visit list && there are no more URLs being processed
// or 
// 2. The max number of PNGs has been found