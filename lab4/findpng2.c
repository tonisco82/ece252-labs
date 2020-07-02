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
// #include <libxml/HTMLparser.h>
// #include <libxml/parser.h>
// #include <libxml/xpath.h>
// #include <libxml/uri.h>

#include "./utils/linkedList.c"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "

#define OUTPUT_FILE "png_urls.txt" //Maybe not needed
#define DEFAULT_THREAD_NUM 1
#define DEFAULT_PNG_NUM 50
#define DEFAULT_LOG_FILE ""

#define DEFAULT_STRING_SZ 200

#define MAX_HTABLE_SZ 500

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
}


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
    Node_t *png_urls = NULL; // The resulting png urls
    Node_t *to_visit = NULL; // A list of the next URLs to Visit. After pop need to ensure the URL has not been checked before (but also check before putting in to save memory)
    struct hsearch_data *visited_urls = calloc(1, sizeof(struct hsearch_data));
    if(hcreate_r(MAX_HTABLE_SZ, visited_urls) == 0){
        printf("Error: insufficient space to create hashtable\n");
        free(logfile);
        free(seed_url);
        free(hsearch_data);
        return -1;
    }

    /** @main_section: Start Main Part of the Program **/





    /** @output_section: Output Timing Result **/

        // Output to logfile if applicable
    if(strcmp(logfile, DEFAULT_LOG_FILE) != 0) linkedlist_to_file(png_urls, logfile);

        // Print Timing
    gettimeofday(&program_end, NULL);
    printf("findpng2 execution time: %.6lf seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));

    /** @final_section: Cleanup **/
    
    hdestroy_r(visited_urls);
    free(visited_urls);
    free(logfile);
    free(seed_url);
    freeMemory(png_urls);
    freeMemory(to_visit);

    return 0;
}