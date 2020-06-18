/**
 * @brief: Connect to a webserver via curl to obtain 50 randomized strips of an image. Concatenate 
 *         all the PNG images into one PNG image. Both a single threaded and multithreaded option is available
 *         The resulting combined PNG should be called all.png.
 * EXAMPLE: ./paster -n 2 -t 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <pthread.h>
#include "helpers.c"
#include "main_write_header_cb.c"
#include "crc.c"
#include "zutil.c"

#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define IMG_URL_SIZE 46
#define IMG_URL_IMG_IND 44 //Index of part of string used to control which image
#define IMG_URL_SERVER_IND 14 //Index of part of string used to control which server to use
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define DUMMYFILE "dummy0.png"
#define DUMMYFILE_LEN 11 // Length of dummy0.png
#define DUMMYFILE_IND 5 // Index of the number in dummy0.png
#define IMAGE_PARTS 50 // Number of parts of the image sent from the serrver
#define NUM_SERVERS 3 // Number of servers
static unsigned int numthreads = 1; //default
static unsigned int numpicture = 1; //default

typedef struct threadargs {
    char url[256]; 
	int threadid;
} *threadargs_p;
unsigned int finishedretrieval = 0;
simple_PNG_p imgs[50];


int singlethread (int numpicture);
int multithread (int threads, int numpicture);
void* retrieve (void* data);

int main(int argc, char *argv[]) { //get args, call function
	int c;
    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
        case 't':
            numthreads = strtoul(optarg, NULL, 10);
            if (numthreads < 1){
            	printf("%s: number of threads must be 1 or more -- 't'\n", argv[0]);
                return -1;
            }
            break;
        case 'n':
            numpicture = strtoul(optarg, NULL, 10);
            if (numpicture < 1 || numpicture > NUM_SERVERS) {
                printf("%s: number of picture must be between 1 and 3 -- 'n'\n", argv[0]);
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    int ret;
    if (numthreads == 1){
    	ret = singlethread(numpicture);
    } else {
    	ret = multithread(numthreads, numpicture);
    }
    return ret;
}

int singlethread (int numpicture){

   	for (int i=0; i<50; i++){
		// Allocate memory for PNG struct
		imgs[i] = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
   		imgs[i]->p_IHDR = NULL;
   		imgs[i]->p_IDAT = NULL;
   		imgs[i]->p_IEND = NULL;
   		imgs[i]->filled = 0;
   		imgs[i]->busy = 0;
   	}
   	//////////////////////////////////////////////////////////////
   	//Set up cURL and retrieve images
   	int retrieved = 0; //num unique pieces obtained
   	char url[256];
   	strcpy(url, IMG_URL);
   	url[44] = (char)(numpicture + '0'); //44 is the offset required to change which image is being retrieved in the url
   	//curl stuff
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
    
    recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }
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

    /////////////////////////////////////////////////////
    //Loop and retrieve data
    while (retrieved < 50){
	    res = curl_easy_perform(curl_handle);
	    if( res != CURLE_OK) {
	        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	    } else {
	         //dummy for debug
			//printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf.size, recv_buf.buf, recv_buf.seq);
	    }
	    if(!imgs[recv_buf.seq]->filled){
	    	if(write_file(DUMMYFILE, recv_buf.buf, recv_buf.size) ||//write data retrieved to intermediate file
	    	get_png(DUMMYFILE, imgs[recv_buf.seq])){ //process data while server sleeps
				imgs[recv_buf.seq]->filled = 0;
				continue;
			}
	    	retrieved++;
	    }
	}

	////////////////////////////////////////////////////
    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);

    ///////////////////////////////////////////////////////////
   	//copied from catbuf, used to concatenate array of pngs

   	U32 height = 0;
   	data_IHDR_p calcs[50];

   	for (int i=0; i<50; i++){  //loop get IHDR data for dimensions
	   	calcs[i] = (data_IHDR_p)malloc(sizeof(struct data_IHDR));
   		int get_data_IHDR_status = get_data_IHDR((char*) imgs[i]->p_IHDR->p_data, calcs[i]);

		if (get_data_IHDR_status != 0) {
			printf("strip %d: failed to get IHDR data\n", i);
			for(int j=0;j<=i;j++){
				free(calcs[j]);
			}
			for(int j=0;j<50;j++){
				free_png(imgs[j]);
			}
	    	return -1;
		}

   		height += calcs[i]->height;
   	}
	
   	U32 width = calcs[0]->width; // Assumes all be the same width

	// **Inflate the Compressed Data and Append to Each Other**

	U8* catbuf = malloc(height*((width*4)+1)); //Buffer
	U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    U64 len_tot = 0;      //running tally of total length
    int ret = 0;        //debug param

   	for (int i=0; i<50; i++){ //loop inflate data
   		len_def = imgs[i]->p_IDAT->length;
   		len_inf = 0;
   		ret = mem_inf(catbuf+len_tot, &len_inf, imgs[i]->p_IDAT->p_data, len_def); //automatically concatenate inflated data to buffer
	    
		if (ret !=0){
	        printf("StdError: mem_def failed. ret = %d.\n", ret);
			for(int j=0;j<50;j++){
				free_png(imgs[j]);
				free(calcs[j]);
			}

	        return ret;
	    }

	    len_tot += len_inf;  //keep running total for buffer offset on next data inflate
    }

	// **Deflate the Uncompressed Data**

    U8* newdata = malloc(height*((width*4)+1)); //new data buffer
	U64 size = (height*((width*4)+1));

    ret = mem_def(newdata, &len_def, catbuf, size, Z_DEFAULT_COMPRESSION);
    if (ret !=0){
        printf("StdError: mem_def failed. ret = %d.\n", ret);
		for(int j=0;j<50;j++){
			free_png(imgs[j]);
			free(calcs[j]);
		}
        return ret;
    }
    free(catbuf);

	// **Creating new PNG Struct**

    simple_PNG_p newpng = (simple_PNG_p)malloc(sizeof(struct simple_PNG));

	newpng->p_IHDR = (chunk_p)malloc(sizeof(struct chunk));
	data_IHDR_p new_header = malloc(sizeof(struct data_IHDR));
	new_header->width = width;
	new_header->height = height;
	new_header->bit_depth = calcs[0]->bit_depth;
	new_header->color_type = calcs[0]->color_type;
	new_header->compression = calcs[0]->compression;
	new_header->filter = calcs[0]->filter;
	new_header->interlace = calcs[0]->interlace;
	fill_data_IHDR(new_header, newpng->p_IHDR);

	newpng->p_IDAT = (chunk_p)(malloc(sizeof(struct chunk)));
	newpng->p_IDAT->length = (U32)len_def;
	newpng->p_IDAT->type[0] = imgs[0]->p_IDAT->type[0];
	newpng->p_IDAT->type[1] = imgs[0]->p_IDAT->type[1];
	newpng->p_IDAT->type[2] = imgs[0]->p_IDAT->type[2];
	newpng->p_IDAT->type[3] = imgs[0]->p_IDAT->type[3];
	newpng->p_IDAT->p_data = newdata;
	newpng->p_IDAT->crc = crccheck(newpng->p_IDAT);

	newpng->p_IEND = imgs[0]->p_IEND; // Re-use same END data structure

   	write_png_file("all.png", newpng);

	for (int i=0; i<50; i++){  //loop to free memory
   		free_png(imgs[i]);
		free(calcs[i]);
   	}
	free(newdata);
	free(newpng->p_IDAT);
	free(newpng->p_IHDR->p_data);
	free(newpng->p_IHDR);
	free(newpng);
	free(new_header);
	remove(DUMMYFILE);
   	return 0;
}

typedef struct thread_data {
	unsigned int* retrieved; //Number of data chunks retrieved
	simple_PNG_p* imagesRetrieved; //Locations of where the data chunks are
	int pthread_id;
	int photo_num;
}  thread_data_t;

void *get_data(void * arg){ //Subrountine to get the data
	//** Decode Input **//
	thread_data_t *p_in = arg;

	//** cURL Setup **//
	CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
  
    recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (curl_handle == NULL) {
        printf("Error: Thread %d-curl_easy_init: returned NULL\n", p_in->pthread_id);
        return  NULL;
    }

	//** Variable Setup **//
	int* ret = malloc(sizeof(int));
	*ret = 0; //track how many packets retrieved

	//	URL string
	char url[IMG_URL_SIZE];
   	strcpy(url, IMG_URL);
   	url[IMG_URL_IMG_IND] = (char)(p_in->photo_num + '0');
	url[IMG_URL_SERVER_IND] = (char)((p_in->pthread_id % 3) + '1'); //Offset of 1 since the server number is 1,2, or 3

	// Dummy File Name
	char temp_file_name[30];
	snprintf(temp_file_name, 30, "%d.png", p_in->pthread_id);
   	//temp_file_name[DUMMYFILE_IND] = (char)(p_in->pthread_id + '0');

	//** cURL More Setup **//
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


	//** Keep Retrieving the Data **//
	while(*(p_in->retrieved) < IMAGE_PARTS){
		res = curl_easy_perform(curl_handle);
	    if( res != CURLE_OK) {
	        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	    } else {
	         //Print how memory received
			//printf("%d:%lu bytes received in memory %p, seq=%d, filled=%d ", p_in->pthread_id, recv_buf.size, recv_buf.buf, recv_buf.seq, p_in->imagesRetrieved[recv_buf.seq]->filled);
	    }

	    if(p_in->imagesRetrieved[recv_buf.seq]->filled == 0 && p_in->imagesRetrieved[recv_buf.seq]->busy == 0){ //If not yet received
			p_in->imagesRetrieved[recv_buf.seq]->filled = 0; //Set to received
			p_in->imagesRetrieved[recv_buf.seq]->busy = 1;

			if (write_file(temp_file_name, recv_buf.buf, recv_buf.size) ||get_png(temp_file_name, p_in->imagesRetrieved[recv_buf.seq])){ //write data retrieved to intermediate file
				 //process data while server sleeps
				p_in->imagesRetrieved[recv_buf.seq]->filled = 0;  //redundancy check, had issue with failing data read, if fail, then repull data from server (dont mark done)
				p_in->imagesRetrieved[recv_buf.seq]->busy = 0;
				continue;
			}
			*(p_in->retrieved) += 1; //Increase total received counter by 1
			p_in->imagesRetrieved[recv_buf.seq]->busy = 0;
			p_in->imagesRetrieved[recv_buf.seq]->filled = 1;
			//printf(" NOT RECEIVED YET-Currently %d parts are retrieved\n", *(p_in->retrieved));
	    }else{ //If received
			//printf(" ALREADY RECEIVED-image section %d is full: %d\n", recv_buf.seq, p_in->imagesRetrieved[recv_buf.seq]->filled);
		}
	}

	//** Cleanup **//
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
	remove(temp_file_name);
	return NULL;
}

/**
 * @brief: Retrieves data with many threads, making an image from the data
 * @return_values:
        0: Success
        -1: Error: Must have at least 1 thread
 */
int multithread (int threads, int numpicture){
	if(threads < 1) return -1;

	//Array of thread ids
	pthread_t *pthread_ids = malloc(sizeof(pthread_t) * threads);
	// Thread data value to pass in;
	thread_data_t thread_input_data[threads];

	//num unique pieces obtained
	unsigned int retr = 0;
	//List of which images were retrieved
	simple_PNG_p img[IMAGE_PARTS];

	for (int i=0; i<IMAGE_PARTS; i++){
		// Allocate memory for PNG struct
		img[i] = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
   		img[i]->p_IHDR = NULL;
   		img[i]->p_IDAT = NULL;
   		img[i]->p_IEND = NULL;
   		img[i]->filled = 0;
   		img[i]->busy = 0;
   	}
	
   	for(int i=0;i<threads;i++){
		//num unique pieces obtained
		thread_input_data[i].retrieved = &retr;
		//List of which images were retrieved
		thread_input_data[i].imagesRetrieved = (simple_PNG_p*) &img;
		thread_input_data[i].pthread_id = i;
		thread_input_data[i].photo_num = numpicture;
		pthread_create(pthread_ids + i, NULL, get_data, (void *) &thread_input_data[i]);
	}

	for(int i=0;i<threads;i++){
		pthread_join(pthread_ids[i], NULL);
		usleep(10);
	}

	//**Make PNG from Result**
   	U32 height = 0;
   	data_IHDR_p calcs[IMAGE_PARTS];

   	for (int i=0; i<IMAGE_PARTS; i++){  //loop get IHDR data for dimensions
	   	calcs[i] = (data_IHDR_p) malloc(sizeof(struct data_IHDR));
		calcs[i]->height = 0;
		calcs[i]->width = 0;
   		int get_data_IHDR_status = get_data_IHDR((char*) img[i]->p_IHDR->p_data, calcs[i]);
		if (get_data_IHDR_status != 0) {
			printf("strip %d: failed to get IHDR data\n", i);
			for(int j=0;j<=i;j++){
				free(calcs[j]);
			}
			for(int j=0;j<IMAGE_PARTS;j++){
				free_png(img[j]);
			}
	    	return -1;
		}

   		height += calcs[i]->height;
   	}
	
   	U32 width = calcs[0]->width; // Assumes all be the same width

	// **Inflate the Compressed Data and Append to Each Other**

	U8* catbuf = malloc(height*((width*4)+1)); //Buffer
	U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    U64 len_tot = 0;      //running tally of total length
    int ret = 0;        //debug param

   	for (int i=0; i<IMAGE_PARTS; i++){ //loop inflate data
   		len_def = img[i]->p_IDAT->length;
   		len_inf = 0;
   		ret = mem_inf(catbuf+len_tot, &len_inf, img[i]->p_IDAT->p_data, len_def); //automatically concatenate inflated data to buffer
	    
		if (ret !=0){
	        printf("StdError: mem_def failed. ret = %d.\n", ret);
			for(int j=0;j<IMAGE_PARTS;j++){
				free_png(img[j]);
				free(calcs[j]);
			}

	        return ret;
	    }

	    len_tot += len_inf;  //keep running total for buffer offset on next data inflate
    }

	// **Deflate the Uncompressed Data**

    U8* newdata = malloc(height*((width*4)+1)); //new data buffer
	U64 size = (height*((width*4)+1));

    ret = mem_def(newdata, &len_def, catbuf, size, Z_DEFAULT_COMPRESSION);
    if (ret !=0){
        printf("StdError: mem_def failed. ret = %d.\n", ret);
		for(int j=0;j<IMAGE_PARTS;j++){
			free_png(img[j]);
			free(calcs[j]);
		}
        return ret;
    }
    free(catbuf);

		// **Creating new PNG Struct**

    simple_PNG_p newpng = (simple_PNG_p)malloc(sizeof(struct simple_PNG));

	newpng->p_IHDR = (chunk_p)malloc(sizeof(struct chunk));
	data_IHDR_p new_header = malloc(sizeof(struct data_IHDR));
	new_header->width = width;
	new_header->height = height;
	new_header->bit_depth = calcs[0]->bit_depth;
	new_header->color_type = calcs[0]->color_type;
	new_header->compression = calcs[0]->compression;
	new_header->filter = calcs[0]->filter;
	new_header->interlace = calcs[0]->interlace;
	fill_data_IHDR(new_header, newpng->p_IHDR);

	newpng->p_IDAT = (chunk_p)(malloc(sizeof(struct chunk)));
	newpng->p_IDAT->length = (U32)len_def;
	newpng->p_IDAT->type[0] = img[0]->p_IDAT->type[0];
	newpng->p_IDAT->type[1] = img[0]->p_IDAT->type[1];
	newpng->p_IDAT->type[2] = img[0]->p_IDAT->type[2];
	newpng->p_IDAT->type[3] = img[0]->p_IDAT->type[3];
	newpng->p_IDAT->p_data = newdata;
	newpng->p_IDAT->crc = crccheck(newpng->p_IDAT);

	newpng->p_IEND = img[0]->p_IEND; // Re-use same END data structure

   	write_png_file("all.png", newpng);

	//End of File Cleanup
	free(pthread_ids);
	for(int i=0;i<IMAGE_PARTS;i++){
		free_png(img[i]);
		free(calcs[i]);
	}
	free(newdata);
	free(newpng->p_IDAT);
	free(newpng->p_IHDR->p_data);
	free(newpng->p_IHDR);
	free(newpng);
	free(new_header);
	remove(DUMMYFILE);
	return 0;
}

void* retrieve(void*data){
	threadargs_p args = (struct threadargs*) data;
	CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
	int* ret = malloc(sizeof(int));
	*ret = 0; //track how many packets retrieved
	char file[30];
	snprintf(file, 30, "%d.png", args->threadid);
	////////////////////////////////////
	//curl setup   
    recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return  ((void*) 1);
    }
    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, args->url);
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

    /////////////////////////////////////////////////////
    //Loop and retrieve data
    while (finishedretrieval == 0){
	    res = curl_easy_perform(curl_handle);
	    if( res != CURLE_OK) {
	        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	    } else {
	         //dummy for debug
			//printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf.size, recv_buf.buf, recv_buf.seq);
	    }
	    if(!imgs[recv_buf.seq]->filled){
			imgs[recv_buf.seq]->filled = true;
			write_file(file, recv_buf.buf, recv_buf.size); //write data retrieved to intermediate file
			get_png(file, imgs[recv_buf.seq]); //process data while server sleeps
			*ret = *ret +1;
	    }
	}

	////////////////////////////////////////////////////
    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
	remove(file);
	return (void*) ret;
}
