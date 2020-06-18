
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/wait.h>
#include <string.h> // For strings
#include <time.h>// For program run time
#include <sys/time.h>
#include <unistd.h> // Sleep
#include "utils/file_utils/file_fns.c"
#include "utils/png_utils/png_fns.c"
#include "utils/util.c"
#include "main_write_header_cb.c"


#define STRIP_WIDTH 400 //Pixel Width of incoming PNG
#define STRIP_HEIGHT 6 //Pixel Height of incoming PNG

#define IMAGE_PARTS 50 // Number of parts of the image sent from the serrver
#define NUM_SERVERS 3 // Number of servers
#define NUM_IMAGES 3 // Number of images
#define NUM_SEMS 2
#define SERVER_PORT 2530 // Port number on the machine
#define URL1 "http://ece252-"
#define URL1_LEN 14
#define URL2 ".uwaterloo.ca:"
#define URL2_LEN 14
#define URL3 "/image?img="
#define URL3_LEN 11
#define URL4 "&part="
#define URL4_LEN 6
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */

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

int producer (int shmid, int tracker, int shmid_sems, int picnum, int numserv);
int consumer (int csleeptime);

int main(int argc, char *argv[]) { //get args, call function
    if(argc != 6){
        printf("Usage example: ./paster 2 1 3 10 1\n");
        return -1;
    }
	const int queuesize = atoi(argv[2]);
	const int numproducers = atoi(argv[3]);
	const int numconsumers = atoi(argv[4]);	
	const int csleeptime = atoi(argv[5]);
	const int picnum = atoi(argv[6]);
	if (queuesize <1 || numproducers <1 || numconsumers<1 || csleeptime < 0 || picnum <1 || picnum > 3){
		printf("invalid arguments");
		return -1;
	}
	pid_t pid=0;
    pid_t cpids[numconsumers];
    pid_t ppids[numproducers];
    struct timeval tv;
        // Start Timer
    struct timeval program_start, program_end;
    if (gettimeofday(&program_start, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    //setup shared memory
    int shmid = shmget(IPC_PRIVATE, BUF_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int tracker = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * NUM_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (shmid == -1 || shmid_sems == -1 || tracker == -1) {
        perror("shmget");
        abort();
    }
    //attach to and initialize shared memory
    sem_t *sems;
    void *buf;
    void* count;
    buf = shmat(shmid, NULL, 0);
    sems = shmat(shmid_sems, NULL, 0);
    count = shmat(tracker, NULL, 0);
    memset(buf, 0, BUF_SIZE);
    memset(tracker, 0, sizeof(int));
    if ( sem_init(&sems[0], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[0])");
        abort();
    }
    if ( sem_init(&sems[1], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[1])");
        abort();
    }

	//spawn producer children
	for ( i = 0; i <numproducers; i++) {        
        pid = fork();
        if ( pid > 0 ) {        /* parent proc */
            ppids[i] = pid;
        } else if ( pid == 0 ) { /* child proc */
        	int numserv = (i % 3)+1
            producer(shmid, picnum, numserv);
            break;
        } else {
            perror("fork");
            abort();
        }        
    }
    //spawn consumer children
    for ( i = 0; i <numconsumers; i++) {       
        pid = fork();
        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
        } else if ( pid == 0 ) { /* child proc */
            consumer(csleeptime);
            break;
        } else {
            perror("fork");
            abort();
        }        
    }
    //clean up children
    if ( pid > 0 ) {            /* parent process */
        for ( i = 0; i <numproducers; i++ ) {
            waitpid(ppids[i], &state, 0);
            if (WIFEXITED(state)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }
        for ( i = 0; i <numconsumers; i++ ) {
            waitpid(cpids[i], &state, 0);
            if (WIFEXITED(state)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }
    }
    //////////////////////////////////////
    //combine PNG files here! & write all.png


    //finish up, get timing
    if ( pid > 0 ) {
        gettimeofday(&program_end, NULL);
        printf("paster2 execution time: %.61f seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));
    }
    return 0;
}


int producer (int shmid, int tracker, int shmid_sems, int picnum, int numserv){
   	//curl stuff
   	int received = 0;
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
    void* mem = shmat(shmid, NULL, 0);
    int* numreceived = shmat(tracker, NULL, 0);
    sem_t* sems = shmat(shmid_sems, NULL, 0);

    
    recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }
    while (received < 50){
    	//check image counter
    	sem_wait(&sems[1]);
    	received = *numreceived;
    	*numreceived++;
    	sem_post(&sems[1]);
	    char* url = createTargetURL(numserv, picnum, received)
	    /* specify URL to get */
	    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	    /* register write call back function to process received data */
	    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
	    /* user defined data structure passed to the call back function */
	    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);
	    /* register header call back function to process received header data */
	    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
	    /* user defined data structure passed to the call back function */
	    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);
	    /* some servers requires a user-agent field */
	    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curl_handle);
        if( res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
    }
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    shmdt(recv_buf);
}


