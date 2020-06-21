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
#include "utils/file_utils/file_fns.c"
#include "utils/png_utils/png_fns.c"
#include "utils/util.c"
#include "main_write_header_cb.c"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define MAX_STRIP_SIZE 10000;
#define NUM_SEMS 3 // Number of Semaphores in use by the system.
// 0: Indication of downloaded image buffer being used, first element is the number of elements currently in the buffer
// 1: Indication of number of images received being used
// 2: Indication of resulting image data being used.

#define STRIP_WIDTH 400 //Pixel Width of incoming PNG
#define STRIP_HEIGHT 6 //Pixel Height of incoming PNG

#define IMAGE_PARTS 50 // Number of parts of the image sent from the server
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
 * @steps: steps are shown below for both the producer and consumer
 * @producers:
1. Check  (wait for sems[1]) to see which image needs to be downloaded
    a. If image needed to be downloaded > IMAGE_PARTS (50) then the exit.
    b. Otherwise increase the number by 1
    c. Post to sems[1]
    d. Download the image part.
2. Check to see if the buffer is full (wait for sems[0]). 
    a. If full, keep checking until it isn't full.
    b. Otherwise, increase it's amount by 1, and add element to the buffer.
    c. Post to sems[0].

 * @consumers:
1. Wait for designated period of time
2. Check to see if there is anything in the buffer (wait sem[0])
    a. If Empty
        i. Check to see if all images are downloaded (wait sems[1]). If it is, exit. Otherwise post sems[1];
        ii. Repeat step 2
    b. Otherwise
        i. Copy an element from the buffer
        ii. Decrease buffer amount by 1.
    c. Post sems[0]
    d. Uncompress/decompress/ whatever needs to be done.
3. Check to see if the final image data is in use (wait sems[2]).
    a. Add data to the final image data.
    b. post sems[2].
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
int consumer (int csleeptime, int shmid_sems, int rec_buff_size, int img_rec_buff, processed_buff_size, processed_img_buff, num_images_received);

int main(int argc, char *argv[]) {
    /** Input Validation and Setup **/
        // Check Inputs
    if(argc != 6){
        printf("Usage example: ./paster 2 1 3 10 1\n");
        return -1;
    }

        //Input Variables
	const int queuesize = atoi(argv[2]);
	const int numproducers = atoi(argv[3]);
	const int numconsumers = atoi(argv[4]);	
	const int csleeptime = atoi(argv[5]);
	const int picnum = atoi(argv[6]);

	if (queuesize <1 || numproducers <1 || numconsumers<1 || csleeptime < 0 || picnum <1 || picnum > NUM_IMAGES){
		printf("invalid arguments");
		return -1;
	}

        //Process Variables
	pid_t pid = 0;
    pid_t cpids[numconsumers];
    pid_t ppids[numproducers];
    int state;

        // Start Timer and Variables
    struct timeval program_start, program_end;
    if (gettimeofday(&program_start, NULL) != 0) {
        perror("gettimeofday");
        return -1;
    }

    /** Setup Shared Memory **/
    const int rec_buff_size = MAX_STRIP_SIZE * queuesize + sizeof(short);
    int img_rec_buff = shmget(IPC_PRIVATE, rec_buff_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Main Buffer for Received Images (after producer, before consumer)

    const int processed_buff_size = IMAGE_PARTS * STRIP_HEIGHT * ((STRIP_WIDTH * 4) + 1);
    int processed_img_buff = shmget(IPC_PRIVATE, processed_buff_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Main Buffer for Processed Images (after consumer)

    int num_images_received = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Integer Tracker

    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * NUM_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Semaphore

    if (img_rec_buff == -1 || shmid_sems == -1 || num_images_received == -1 || processed_img_buff == -1) {
        perror("shmget");
        return -1;
    }

        //attach to shared memory
    sem_t *sems;
    void *rec_buff;
    void *count;
    void *proc_buff;

    img_rec_buff = shmat(img_rec_buff, NULL, 0);
    sems = shmat(shmid_sems, NULL, 0);
    count = shmat(num_images_received, NULL, 0);
    proc_buff = shmat(processed_img_buff, NULL, 0);

    if ( img_rec_buff == (void *) -1 || sems == (void *) -1 || count == (void *) -1) {
        perror("shmat");
        return -1;
    }
        // Initialize Shared Memory
    memset(img_rec_buff, 0, rec_buff_size);
    memset(num_images_received, 0, sizeof(int));
    memset(processed_img_buff, 0, processed_buff_size);
    if ( sem_init(&sems[0], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[0])");
        return -1;
    }
    if ( sem_init(&sems[1], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[1])");
        return -1;
    }

    /** Initializing Child Processes **/
        //spawn consumer children. Note: spawn consumers first since they have a sleep time
    for ( i = 0; i < numconsumers; i++) {       
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
	    //spawn producer children
    if ( pid > 0 ) {  
        for ( i = 0; i < numproducers; i++) {        
            pid = fork();
            if ( pid > 0 ) {        /* parent proc */
                ppids[i] = pid;
            } else if ( pid == 0 ) { /* child proc */
                producer(img_rec_buff, picnum, (i % NUM_SERVERS) + 1);
                break;
            } else {
                perror("fork");
                abort();
            }        
        }
    }

    /** ONLY Parent Process does this part **/
    if ( pid > 0 ) {
        //wait for and clean up children
        for ( i = 0; i < numproducers; i++ ) {
            waitpid(ppids[i], &state, 0);
            if (WIFEXITED(state)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }
        for ( i = 0; i < numconsumers; i++ ) {
            waitpid(cpids[i], &state, 0);
            if (WIFEXITED(state)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }
    
        /** Combine Data into 1 PNG struct **/

            //Creation of Empty PNG
        simple_PNG_p resulting_png;
        if(create_empty_png(&resulting_png) != 0){
            perror("create_empty_png");
            abort();
        }

            //Setting IDAT Chunk
        resulting_png->p_IDAT->p_data = (U8 *) proc_buff;
        resulting_png->p_IDAT->length = STRIP_HEIGHT*IMAGE_PARTS*((STRIP_WIDTH*4)+1);
        resulting_png->p_IDAT->crc = crc_generator(resulting_png->p_IDAT);

            //Setting IHDR Chunk

                //Convert into data_IHDR struct
        struct data_IHDR resulting_png_IHDR;
        if(fill_IHDR_data(&resulting_png_IHDR, resulting_png->p_IHDR) != 0){
            perror("fill_IHDR_data");
            abort();
        }
                //Update Values
        resulting_png_IHDR.length = STRIP_HEIGHT*IMAGE_PARTS;
        resulting_png_IHDR.width = STRIP_WIDTH;

                //Convert Back into Chunk
        free(resulting_png->p_IHDR->p_data); //Only need to free the data pointer, the rest will be re-used
        if(fill_IHDR_chunk(resulting_png->p_IHDR, &resulting_png_IHDR) != 0){
            perror("fill_IHDR_chunk");
            abort(); 
        }

        /** Write PNG struct to output file **/

        unsigned long png_data_size = 0;
        void *png_data;
        if(fill_png_data(&png_data, &png_data_size, simple_PNG_p png_data) != 0){
            perror("fill_png_data");
            abort();
        }
        if(write_mem_to_file(png_data, png_data_size, RESULT_PNG_NAME) != 0){
            perror("write_mem_to_file");
            abort();
        }

        /** Get Program Timing **/

        gettimeofday(&program_end, NULL);
        printf("paster2 execution time: %.61f seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));

        /** Cleanup **/
        
        free_simple_PNG(resulting_png);
        free(png_data);
    }

    /** Cleanup **/
    if ( shmdt(img_rec_buff) != 0 ) {
        perror("shmdt");
        abort();
    }

    if ( shmdt(shmid_sems) != 0 ) {
        perror("shmdt");
        abort();
    }

    if ( shmdt(num_images_received) != 0 ) {
        perror("shmdt");
        abort();
    }

    if ( shmdt(processed_img_buff) != 0 ) {
        perror("shmdt");
        abort();
    }

        // Destroy Semaphore
    if( pid > 0){
        for(int i = 0; i < NUM_SEMS; i++){
            if (sem_destroy(&sems[i])) {
                perror("sem_destroy");
                abort();
            }
        }
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
    while (received < IMAGE_PARTS){
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
};

/**
 * @brief: Function to Run the Consumer Portion
 * @params:
 * cssleeptime: sleep time of each consumer between the processing of individual image segments.
 * @return:
 * -1: Error
 * 0: Success
 */
int consumer (int csleeptime, int shmid_sems, int rec_buff_size, int img_rec_buff, processed_buff_size, processed_img_buff, num_images_received){
    /** Setup **/
    int num_recv = 0;
    int num_in_buffer = 0;
    sem_t *sems shmat(shmid_sems, NULL, 0);
    void *rec_buff = shmat(img_rec_buff, NULL, 0);
    void *num_imgs_recv = shmat(num_images_received, NULL, SHM_RDONLY);
    void *proc_buff = shmat(processed_img_buff, NULL, 0);

    while(num_recv < IMAGE_PARTS || num_in_buffer > 0){
        /** Wait for designated period of time **/
        usleep(csleeptime);

        /** @critical_section: Check to see if there is anything in the buffer **/
        sem_wait(&sems[0]);

        sem_post(&sems[0]);
}