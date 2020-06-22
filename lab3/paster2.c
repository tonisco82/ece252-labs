#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/wait.h>
#include <string.h> // For strings
#include <time.h> // For program run time
#include <sys/time.h> // USleep function
#include <sys/shm.h> // Shared Memory
#include <semaphore.h> // Semaphores

#include "utils/file_utils/file_fns.c" // File input/output functions
#include "utils/png_utils/png_fns.c" // PNG functions
#include "utils/util.c" // Basic functions
#include "utils/cURL/curl_fns.c" // curl functions

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */

#define STRIP_WIDTH 400 //Pixel Width of incoming PNG
#define STRIP_HEIGHT 6 //Pixel Height of incoming PNG
#define PROC_BUFF_ELEMENT_SZ (STRIP_HEIGHT * ((STRIP_WIDTH * 4) + 1)) //The size of data chunk of each PNG strip
#define MAX_STRIP_SIZE 10000
#define RECV_BUFF_ELEMENT_SZ (MAX_STRIP_SIZE + 2*sizeof(unsigned long)) //Size of each buffer element in the recv buffer
#define NUM_SEMS 3 // Number of Semaphores in use by the system.
// 0: Indication of downloaded image buffer being used, first element is the number of elements currently in the buffer
// 1: Indication of next image to download being used
// 2: Indication of resulting image data being used.

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

#define SEM_PROC 1 // Share the Semaphore with Other Processes

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
NOTE: The recv buff acts like a stack. Append and remove from the end.

 * @consumers:
1. Wait for designated period of time
2. Check to see if there is anything in the buffer (wait sem[0])
    a. If Empty
        i. Check to see if all images are downloaded and processed (wait sems[2]). If it is, exit. Otherwise post sems[2];
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
 * heap allocated string pointer to the url. Is null terminated. User must de-allocated.
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

int producer (int img_rec_buff, int num_images_received, int shmid_sems, int picnum, int numserv, int queuesize);
int consumer (int csleeptime, int shmid_sems, int img_rec_buff, int processed_img_buff);

int main(int argc, char *argv[]) {
    /** Input Validation and Setup **/
        // Check Inputs
    if(argc != 6){
        printf("Usage example: ./paster2 2 1 3 10 1\n");
        return -1;
    }

        //Input Variables
	const int queuesize = atoi(argv[1]);
	const int numproducers = atoi(argv[2]);
	const int numconsumers = atoi(argv[3]);	
	const int csleeptime = atoi(argv[4]);
	const int picnum = atoi(argv[5]);

	if (queuesize <1 || numproducers <1 || csleeptime < 0 || picnum <1 || picnum > NUM_IMAGES){ //numconsumers<1 || 
		printf("invalid arguments\n");
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
        // This is the receiving buffer. First is a short with the number of elements in the buffer, followed by an array of MAX_STRIP_SIZE elements, 
        // each with a integer (long) size followed by integer (long) part number followed by the data.
        // Acts like a stack. Take the num of elements in the buff (first sizeof(short) bytes) to determine number of elements in the buff. Add/remove from that element.
        // short (2-bytes)-> number of elements currently in the buffer | queuesize groups of RECV_BUFF_ELEMENT_SZ bytes
    const int rec_buff_size = RECV_BUFF_ELEMENT_SZ * queuesize + sizeof(short);
    int img_rec_buff = shmget(IPC_PRIVATE, rec_buff_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Main Buffer for Received Images (after producer, before consumer)

        // short (2-bytes)-> number of elements currently successfully processed | queuesize groups of RECV_BUFF_ELEMENT_SZ bytes (each one is an inflated image part).
    const int processed_buff_size = IMAGE_PARTS * PROC_BUFF_ELEMENT_SZ  + sizeof(short);
    int processed_img_buff = shmget(IPC_PRIVATE, processed_buff_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //Main Buffer for Processed Images (after consumer)

        // Records the an indication for the next image to be received from the server.
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

    rec_buff = shmat(img_rec_buff, NULL, 0);
    sems = shmat(shmid_sems, NULL, 0);
    count = shmat(num_images_received, NULL, 0);
    proc_buff = shmat(processed_img_buff, NULL, 0);

    if ( rec_buff == (void *) -1 || sems == (void *) -1 || count == (void *) -1 || proc_buff == (void *) -1) {
        perror("shmat");
        return -1;
    }
        // Initialize Shared Memory
    memset(rec_buff, 0, rec_buff_size);
    memset(count, 0, sizeof(int));
    memset(proc_buff, 0, processed_buff_size);
    for( int i = 0; i < NUM_SEMS; i++){
        if ( sem_init(&sems[i], SEM_PROC, 1) != 0 ) {
            printf("sem_init(sem[%d])\n", i);
            return -1;
        }
    }

    /** Initializing Child Processes **/
        //spawn consumer children. Note: spawn consumers first since they have a sleep time
    for ( int i = 0; i < numconsumers; i++) {       
        pid = fork();
        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
        } else if ( pid == 0 ) { /* child proc */
            consumer(csleeptime, shmid_sems, img_rec_buff, processed_img_buff);
            break;
        } else {
            perror("fork consumers");
            abort();
        }        
    }
	    //spawn producer children
    if ( pid > 0 ) {  
        for ( int i = 0; i < numproducers; i++) {        
            pid = fork();
            if ( pid > 0 ) {        /* parent proc */
                ppids[i] = pid;
            } else if ( pid == 0 ) { /* child proc */
                producer(img_rec_buff, num_images_received, shmid_sems, picnum, (i % NUM_SERVERS) + 1, queuesize);
                break;
            } else {
                perror("fork producers");
                abort();
            }        
        }
    }

    /** ONLY Parent Process does this part **/
    if ( pid > 0 ) {
        //wait for and clean up children
        for ( int i = 0; i < numproducers; i++ ){
            waitpid(ppids[i], &state, 0);
            if (WIFEXITED(state)) {
                //printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }

        for ( int i = 0; i < numconsumers; i++ ){
            waitpid(cpids[i], &state, 0);
            if (WIFEXITED(state)) {
                //printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], state);
            }             
        }
    
        /** Combine Data into 1 PNG struct **/
            // Deflate Data
        U8* defbuf = (U8 *) malloc(PROC_BUFF_ELEMENT_SZ * IMAGE_PARTS);
        U64 len_inf;
        if(mem_def(defbuf, &len_inf, (U8 *)(proc_buff + sizeof(short)), PROC_BUFF_ELEMENT_SZ * IMAGE_PARTS, Z_DEFAULT_COMPRESSION) != 0){
            perror("mem_def");
            return -1;
        }

            // Creation of Empty PNG
        simple_PNG_p resulting_png;
        if(create_empty_png(&resulting_png) != 0){
            perror("create_empty_png");
            return -1;
        }

            //Setting IDAT Chunk
        resulting_png->p_IDAT->p_data = defbuf;
        resulting_png->p_IDAT->length = PROC_BUFF_ELEMENT_SZ * IMAGE_PARTS;
        resulting_png->p_IDAT->crc = crc_generator(resulting_png->p_IDAT);

            //Setting IHDR Chunk

                //Convert into data_IHDR struct
        struct data_IHDR resulting_png_IHDR;
        if(fill_IHDR_data(&resulting_png_IHDR, resulting_png->p_IHDR) != 0){
            perror("fill_IHDR_data");
            return -1;
        }
                //Update Values
        resulting_png_IHDR.height = STRIP_HEIGHT*IMAGE_PARTS;
        resulting_png_IHDR.width = STRIP_WIDTH;

                //Convert Back into Chunk
        free(resulting_png->p_IHDR->p_data); //Only need to free the data pointer, the rest will be re-used
        if(fill_IHDR_chunk(resulting_png->p_IHDR, &resulting_png_IHDR) != 0){
            perror("fill_IHDR_chunk");
            return -1;
        }

        /** Write PNG struct to output file **/

        unsigned long png_data_size = 0;
        void *png_data;
        if(fill_png_data(&png_data, &png_data_size, resulting_png) != 0){
            perror("fill_png_data");
            return -1;
        }
        if(write_mem_to_file(png_data, png_data_size, RESULT_PNG_NAME) != 0){
            perror("write_mem_to_file");
            return -1;
        }

        /** Get Program Timing **/

        gettimeofday(&program_end, NULL);
        printf("paster2 execution time: %.6lf seconds\n", (double)((double)(program_end.tv_sec - program_start.tv_sec) + (((double)(program_end.tv_usec - program_start.tv_usec)) / 1000000.0)));

        /** Cleanup **/
        
        free_simple_PNG(resulting_png);
        free(png_data);
    }

    /** Cleanup **/
    if ( shmdt(rec_buff) != 0 ) {
        perror("shmdt rec_buff");
        abort();
    }

    if ( shmdt(sems) != 0 ) {
        perror("shmdt sems");
        abort();
    }

    if ( shmdt(count) != 0 ) {
        perror("shmdt count");
        abort();
    }

    if ( shmdt(proc_buff) != 0 ) {
        perror("shmdt proc_buff");
        abort();
    }

        // Destroy Semaphore
    if( pid > 0){
        for(int i = 0; i < NUM_SEMS; i++){
            if (sem_destroy(&sems[i])) {
                perror("sem_destroy");
                return -1;
            }
        }
    }
    return 0;
}

/**
 * @brief: Function to Run the Producer Portion.
 * This includes receiving image data from the server, and the placing the image data into the received images buffer.
 * Once all images are received, the producer stops running.
 * Made so that many producers can be run concurrently to speed up the data-recollection. 
 * @params:
 * img_rec_buff: Shared memory id for a buffer to place all of the received images into.
 * num_images_received: Shared memory id for storing the next image to fetch from the server.
 * shmid_sems: Shared memory id for the semaphores.
 * picnum: The picture number to request
 * numserv: The server to request the picture number from.
 * queuesize: The maximum stack size of the img_rec_buffer.
 * @return:
 * -1: Error
 * 0: Success
 */
int producer (int img_rec_buff, int num_images_received, int shmid_sems, int picnum, int numserv, int queuesize){

   	/** Setup **/
        //Local Variables
   	int received = 0;
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF *recv_buf;
    int num_in_buffer = 0;
    short filled_in_queue = 0;
    unsigned long element_siz = 0;
    unsigned long element_num = 0;
    char* url;

        //Attaching The Shared Memory
    void* shbuf = shmat(img_rec_buff, NULL, 0);
    int* numreceived = shmat(num_images_received, NULL, 0);
    sem_t* sems = shmat(shmid_sems, NULL, 0);

    /** Main Loop **/
    while (received < IMAGE_PARTS){

        /** @critical_section: Check image counter **/
    	sem_wait(&sems[1]);
    	received = *numreceived;
        if(received < IMAGE_PARTS) (*numreceived)++;
    	sem_post(&sems[1]);

    	if (received < IMAGE_PARTS){
            /** cURL Setup **/
            recv_buf = (RECV_BUF *) malloc(sizeof_shm_recv_buf(MAX_STRIP_SIZE));
                    //Initialize GET buffer and cURL
            shm_recv_buf_init(recv_buf, MAX_STRIP_SIZE);
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl_handle = curl_easy_init();

            if (curl_handle == NULL) {
                perror("curl_easy_init");
                abort();
            }
            
    		/** Download Image **/
                /* Create URL */
    		url = createTargetURL(numserv, picnum, received);
		        /* specify URL to get */
            curl_easy_setopt(curl_handle, CURLOPT_URL, url);
                /* register write call back function to process received data */
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 
                /* user defined data structure passed to the call back function */
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)recv_buf);
                /* register header call back function to process received header data */
            curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
                /* user defined data structure passed to the call back function */
            curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)recv_buf);
                /* some servers requires a user-agent field */
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
                /* request and download data */
	        res = curl_easy_perform(curl_handle);

	        if(res != CURLE_OK) {
	            perror("curl_easy_perform()");
                abort();
	        }

            /** @critical_section: Wait For Open Spot in Queue to Copy Memory in (Busy Waiting) **/
            filled_in_queue = 0;
            element_num = (unsigned long) received;
            element_siz = (unsigned long) (recv_buf->size > MAX_STRIP_SIZE ? MAX_STRIP_SIZE : recv_buf->size);
        	do {
		    	sem_wait(&sems[0]);
		        num_in_buffer = *(short *)shbuf;
                if(num_in_buffer < queuesize){
                    filled_in_queue = 1; // Get out of loop
                    (*(short *)shbuf)++; // Increase buffer counter
                    //Copy over the Data
                    memcpy((void *)(shbuf + sizeof(short) + (num_in_buffer) * RECV_BUFF_ELEMENT_SZ ), (void *) &element_siz, sizeof(unsigned long));
	                memcpy((void *)(shbuf + sizeof(short) + (num_in_buffer) * RECV_BUFF_ELEMENT_SZ + sizeof(unsigned long)), (void *) &element_num, sizeof(unsigned long));
	                memcpy((void *)(shbuf + sizeof(short) + (num_in_buffer) * RECV_BUFF_ELEMENT_SZ + (2 * sizeof(unsigned long))), (void *) recv_buf->buf, element_siz);
                }
		        sem_post(&sems[0]);
		    } while (!filled_in_queue);

            /** Clean-Up **/
            free(url);
            curl_easy_cleanup(curl_handle);
            curl_global_cleanup();
            free(recv_buf);
    	}
    }
    return 0;
};

/**
 * @brief: Function to Run the Consumer Portion
 * @params:
 * cssleeptime: sleep time of each consumer between the processing of individual image segments.
 * shmid_sems: shared memory id for the semaphore
 * img_rec_buff: shared memory id for the received image buffer
 * processed_img_buff: shared memory id for the processed images buffer
 * @return:
 * -1: Error
 * 0: Success
 */
int consumer (int csleeptime, int shmid_sems, int img_rec_buff, int processed_img_buff){

    /** Setup **/
    int num_recv = 0;
    short num_in_buffer = 0;
    unsigned long part_number = 0;
    unsigned long part_size = 0;
    void *part_data;
    simple_PNG_p part_png;
    U64 len_inf;
    U8* catbuf = (U8 *) malloc(PROC_BUFF_ELEMENT_SZ);

    sem_t *sems = shmat(shmid_sems, NULL, 0);
    void *rec_buff = shmat(img_rec_buff, NULL, 0);
    void *proc_buff = shmat(processed_img_buff, NULL, 0);

    while(num_recv < IMAGE_PARTS){
        /** Wait for designated period of time **/
        usleep(csleeptime);

        /** @critical_section: Check to see if there is anything in the buffer **/
        sem_wait(&sems[0]);
        num_in_buffer = *(short *)rec_buff;
        if(num_in_buffer != 0){ //There was an element in the buffer, copying out of the buffer
            (*(short *)rec_buff)--; //Decrement number of elements in the buffer
            part_size = *(unsigned long *)(rec_buff + sizeof(short) + (num_in_buffer - 1) * RECV_BUFF_ELEMENT_SZ);
            part_data = malloc(part_size);
            part_number = *(unsigned short *)(rec_buff + sizeof(short) + (num_in_buffer - 1) * RECV_BUFF_ELEMENT_SZ + sizeof(unsigned long));
            memcpy(part_data, (void *)(rec_buff + sizeof(short) + ((num_in_buffer - 1) * RECV_BUFF_ELEMENT_SZ) + (2 * sizeof(unsigned long))), part_size);
        }
        sem_post(&sems[0]);

        if(num_in_buffer == 0){

            /** @critical_section: Check to see if all images are received **/
            sem_wait(&sems[2]);
            num_recv = *(short *)proc_buff;
            sem_post(&sems[2]);

        }else {

            /** Format Into PNG **/
            part_png = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
            if(fill_png_struct(part_png, part_data, part_size) != 0){
                /**
                 * @alert: This section just prevents the code from failing. If an error occurs with the image, it replaces that section with an empty PDF instead of causing the entire process to fail.
                 * Can be taken out, but should be left in for now to stop the process from terminating or hanging.
                 */
                perror("fill_png_struct");
                //abort(); //Uncomment if you would rather have the process terminate

                /** @critical_section: Pass an empty PNG part into the inflated data. **/
                sem_wait(&sems[2]);
                (*(short *)proc_buff)++;
                num_recv = *(short *)proc_buff;
                sem_post(&sems[2]);
            }else{

                if(mem_inf(catbuf, &len_inf, part_png->p_IDAT->p_data, part_png->p_IDAT->length) != 0){
                    perror("mem_inf");
                    abort();
                }

                /** @critical_section: Inflating Data to the Processed Images Buffer **/
                sem_wait(&sems[2]);
                (*(short *)proc_buff)++;
                num_recv = *(short *)proc_buff;
                memcpy((void *)(proc_buff + sizeof(short) + (part_number * PROC_BUFF_ELEMENT_SZ)), (void *)catbuf, len_inf);
                sem_post(&sems[2]);

                free_simple_PNG(part_png);
            }

            /** Clean-up **/
            free(part_data);
        }
    }
    
    free(catbuf);
    return 0;
}