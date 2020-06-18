#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h> // For strings
#include <time.h> // For program run time
#include <unistd.h> // Sleep
#include "utils/file_utils/file_fns.c"
#include "utils/png_utils/png_fns.c"
#include "utils/util.c"

#define STRIP_WIDTH 400 //Pixel Width of incoming PNG
#define STRIP_HEIGHT 6 //Pixel Height of incoming PNG

#define IMAGE_PARTS 50 // Number of parts of the image sent from the serrver
#define NUM_SERVERS 3 // Number of servers
#define NUM_IMAGES 3 // Number of images
#define SERVER_PORT 2530 // Port number on the machine
#define URL1 "http://ece252-"
#define URL1_LEN 14
#define URL2 ".uwaterloo.ca:"
#define URL2_LEN 14
#define URL3 "/image?img="
#define URL3_LEN 11
#define URL4 "&part="
#define URL4_LEN 6

#define RESULT_PNG_NAME "all.png"

/**
 * @brief: Limited Buffer Produce/Consumer problem to downloading an image
 */

/**
 * @brief: Creates the URL for retrieving the image section
 * @params:
 * server_num: int between 1 and NUM_SERVERS
 * image_num: int between 1 and NUM_IMAGES
 * part_num: int between 1 and IMAGE_PARTS
 * @return:
 * heap allocated string pointer to the url. Is null terminated. User must de-allocate.
 */
char *createTargetURL(int server_num, int image_num, int part_num){
    if(server_num > NUM_SERVERS || server_num < 1) server_num = 1;
    if(image_num > NUM_IMAGES || image_num < 1) image_num = 1;
    if(part_num > IMAGE_PARTS || part_num < 1) part_num = 1;

    char snum[10];
    int server_num_len = lenOfNumber(server_num);
    int image_num_len = lenOfNumber(image_num);
    int part_num_len = lenOfNumber(part_num);
    int port_len = lenOfNumber(SERVER_PORT);
    unsigned long url_len = 1 + URL1_LEN + URL2_LEN + URL3_LEN + URL4_LEN + server_num_len + image_num_len + part_num_len + port_len;

    char * result = (char *)malloc(url_len * (sizeof(char)));
    memset((void *)result, (int) '\0', url_len * (sizeof(char)));

    int offset = 0;
    strcpy((char *)(result + offset), URL1);
    offset += URL1_LEN;

    sprintf(snum, "%d", server_num);
    memcpy((void *)(result + offset), (void *)(snum), server_num_len);
    offset += server_num_len;

    strcpy((char *)(result + offset), URL2);
    offset += URL2_LEN;

    sprintf(snum, "%d", SERVER_PORT);
    memcpy((void *)(result + offset), (void *)(snum), port_len);
    offset += port_len;

    strcpy((char *)(result + offset), URL3);
    offset += URL3_LEN;

    sprintf(snum, "%d", image_num);
    memcpy((void *)(result + offset), (void *)(snum), image_num_len);
    offset += image_num_len;

    strcpy((char *)(result + offset), URL4);
    offset += URL4_LEN;

    sprintf(snum, "%d", part_num);
    memcpy((void *)(result + offset), (void *)(snum), part_num_len);
    offset += part_num_len;

    return result;
}

int main(int argc, char *argv[]){
    /** Input Validation and Setup **/
        // Check Inputs
    if(argc != 6){
        printf("Usage example: ./paster 2 1 3 10 1\n");
        return -1;
    }

        // Setup variables
    const int buffer_size = atoi(argv[1]);
    const int num_produces = atoi(argv[2]);
    const int num_consumers = atoi(argv[3]);
    const int consumer_sleep_time = atoi(argv[4]);
    const int image_number = atoi(argv[5]);

        // Start Timer
    struct timeval program_start, program_end;
    if (gettimeofday(&program_start, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }

    /** Cleanup **/

        // Output logging (only parent logs)
    if ( pid > 0 ) {
        gettimeofday(&program_end, NULL);
        printf("paster2 execution time: %.61f seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));
    }
    return 0;
}