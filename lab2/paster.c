/**
 * @brief: Connect to a webserver via curl to obtain 50 randomized strips of an image. Concatenate 
 *         all the PNG images into one PNG image. Both a single threaded and multithreaded option is available
 *         The resulting combined PNG should be called all.png.
 * EXAMPLE: ./catpng ./img1.png ./png/img2.png
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
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define DUMMYFILE "dummy0.png"
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
            if (numpicture < 1 || numpicture > 3) {
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
   	url[44] = (char)(numpicture + '0');
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
			printf("%lu bytes received in memory %p, seq=%d.\n", \
	        		recv_buf.size, recv_buf.buf, recv_buf.seq);
	    }
	    if(!imgs[recv_buf.seq]->filled){
	    	write_file(DUMMYFILE, recv_buf.buf, recv_buf.size); //write data retrieved to intermediate file
	    	get_png(DUMMYFILE, imgs[recv_buf.seq]); //process data while server sleeps
	    	retrieved++;
			printf("%d retrieved\n", retrieved);
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

int multithread (int threads, int numpicture){

   	for (int i=0; i<50; i++){
		// Allocate memory for PNG struct
		imgs[i] = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
   		imgs[i]->p_IHDR = NULL;
   		imgs[i]->p_IDAT = NULL;
   		imgs[i]->p_IEND = NULL;
   		imgs[i]->filled = 0;
   		imgs[i]->busy = 0;
   	}
	pthread_t * p_tids[threads-1];
	for(int i=0; i<threads-1; i++){
		p_tids[i] = malloc(sizeof(pthread_t));
	}
	threadargs_p  input[threads-1];
	int * res [threads-1];

   	//////////////////////////////////////////////////////////////
   	//Set up cURL and retrieve images
   	int retrieved = 0; //num unique pieces obtained
   	char url[256];
   	strcpy(url, IMG_URL);
   	url[44] = (char)(numpicture + '0');

   	for (int i=0; i<threads-1; i++){ //spawn threads up to threads-1 (last is parent)
	   	input[i]->threadid = i;
		url[14] = (char)('1'+ (i & 3)); //distribute work among all 3 servers
		strcpy(input[i]->url, url);
		printf("%s\n", input[i]->url);
		pthread_create(p_tids[i], NULL, retrieve, input[i]);
	}
	bool flag = false;
	while (finishedretrieval == 0){
		flag = true;
		for(int i=0; i<50; i++){
			if (!(imgs[i]->filled)){
				flag = false;
			}
		}
		if (flag){
			finishedretrieval = 1;
		}
	}
	for (int i=0; i<threads-1; i++) {
        pthread_join(*p_tids[i], (void **)&(res[i]));
        retrieved += *res[i];
    }
	if(retrieved != 50){
		printf("error in data retrieval, not all packets successfully retrieved");
		return -1;
	}

	//cleanup
    free(p_tids);
	for (int i=0; i<threads-1; i++) {
        free(res[i]);
    }
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
   	return 0;
}


void* retrieve(void*data){
	printf("why\n");
	threadargs_p args = (struct threadargs*) data;
	CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
	int* ret = malloc(sizeof(int));
	*ret = 0; //track how many packets retrieved
	char file[30];
	printf("%d\n", args->threadid);
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
			printf("%lu bytes received in memory %p, seq=%d.\n", \
	        		recv_buf.size, recv_buf.buf, recv_buf.seq);
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
	free(args);
	remove(file);
	return (void*) ret;
}
