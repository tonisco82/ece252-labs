/**
 * @brief: Given a directory of PNG files, (each PNG file is a horizontal strip of 
 *         the same width, but differing heights, that form a larger image) concatenate 
 *         all the PNG images into one PNG image. The files are named according to the 
 *         convention *_N.png where N is a series of consecutive increasing numbers 
 *         (0, 1, 2, 3, 4, ...), which indicate the position of the image from top to bottom.
 *         The resulting combined PNG should be called all.png.
 * EXAMPLE: ./catpng ./img1.png ./png/img2.png
<<<<<<< Updated upstream
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "helpers.c"
#include "./starter/png_util/crc.c"
#include "./starter/png_util/zutil.c"

unsigned long crccheck (struct chunk* data){
	// Length of the crc buffer
	int len = data->length + 4;

	// Declare crc buffer
	char * restrict chunk_crc_data = (char *)malloc(len);

	// Copy type field into buffer
	strcpy(chunk_crc_data, (char *)data->type);
	// Copy data field into buffer
	for(int i=0;i<data->length;i++){
		*(chunk_crc_data + 4 + i) = (char) *(data->p_data + i);
	}

	// Run crc comparison
	unsigned long testcrc = crc((unsigned char *)chunk_crc_data, len);

	// Test if crc is the same or not
	if (testcrc != data->crc){
		// Returns what the crc should be upon failure
		return  testcrc;
	}

	return (unsigned long) 0;  //returns 0 for pass
}

int main(int argc, char *argv[]) {

    if(argc <= 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }

	simple_PNG_p imgs[argc-1];;

   	int get_png_status = 1;

   	for (int i=0; i<(argc-1); i++){
		imgs[i] = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
   		get_png_status = get_png(argv[i+1], imgs[i]);
   		if (get_png_status != 0) {
			printf("%s: failed to get png\n", argv[i+1]);
	    	return get_png_status;
		} 
   	}

   	int height = 0;
   	int width = 0;
   	data_IHDR_p calcs[argc-1];

   	for (int i=0; i<(argc-1); i++){  //loop get IHDR data for dimensions
	   	calcs[i] = (data_IHDR_p)malloc(sizeof(struct data_IHDR));
   		int get_data_IHDR_status = get_data_IHDR((char*) imgs[i]->p_IHDR->p_data, calcs[i]);
		if (get_data_IHDR_status != 0) {
			printf("%s: failed to get IHDR data\n", argv[i+1]);
			free(calcs);
			free(imgs);
	    	return get_png_status;
		}  	
   		height += calcs[i]->height;
   	}
	
   	width = calcs[0]->width;
	printf("%d height %d width\n", height, width);

	U8* catbuf = malloc(height*((width*4)+1));
	U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    U64 len_tot = 0;      //running tally of total length
    int ret = 0;        //debug param

   	for (int i=0; i<(argc-1); i++){ //loop inflate data
   		len_def = imgs[i]->p_IDAT->length;
   		len_inf = 0;
   		ret = mem_inf(catbuf+len_tot, &len_inf, imgs[i]->p_IDAT->p_data, len_def); //automatically concatenate inflated data to buffer
	    if (ret !=0){
	        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
	        return ret;
	    }
	    len_tot += len_inf;  //keep running total for buffer offset on next data inflate
		printf("%lu len_inf %lu len_tot\n", len_inf, len_tot);
    }

    U8* newdata = malloc(height*((width*4)+1)); //new data buffer
	U64 size = (height*((width*4)+1));

    //deflate data to new data buffer
    ret = mem_def(newdata, &len_def, catbuf, size, Z_DEFAULT_COMPRESSION);
    if (ret !=0){
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
		free(calcs);
		free(imgs);
        return ret;
    }
    free(catbuf);
    // now we should have the proper compressed IDAT data
    //from now on use initial png to create new one
    U32 newheight = (U32)height;

    simple_PNG_p newpng = imgs[0];
	//NOTE: CODE WORKS UP TO THIS POINT TO THE BEST OF MY KNOWLEDGE, NEED TO FIX DATA UPDATE AND WRITE
	//DATA UPDATING (BELOW) COMMENTED O TEST REWRITING INITIAL FILE
	/*
    *(newpng->p_IHDR->p_data+4) = htonl(newheight); 
    newpng->p_IDAT->p_data = newdata;
    newpng->p_IDAT->length = len_def;
    newpng->p_IDAT->crc = crccheck(newpng->p_IDAT);
    newpng->p_IHDR->crc = crccheck(newpng->p_IHDR);
	*/
    //we should now have newpng with all of the data we need
    //now adjust data for easy writing
    //htonl(newpng->p_IDAT->length);
    //htonl(newpng->p_IDAT->crc);
    //htonl(newpng->p_IHDR->length);
    //htonl(newpng->p_IHDR->crc);

   	FILE *fpw;
	fpw = fopen ("all.png", "wb+");
	//write png header
	int png_header[8] = {-119, 80, 78, 71, 13, 10, 26, 10};
	for (int i=0; i<8; i++){
		fwrite(&png_header[i], 1, 1, fpw);  //this works for sure
	}
	//write IHDR
	fwrite(&(newpng->p_IHDR->length), sizeof(newpng->p_IHDR->length), 1, fpw); //pretty sure the size and type are writing properly
	fwrite(&(newpng->p_IHDR->type[0]), sizeof(newpng->p_IHDR->type), 1, fpw);
	fwrite(newpng->p_IHDR->p_data, newpng->p_IHDR->length, 1, fpw);
	fwrite(&(newpng->p_IHDR->crc), sizeof(newpng->p_IHDR->crc), 1, fpw);
	//write IDAT
	fwrite(&(newpng->p_IDAT->length), sizeof(newpng->p_IDAT->length), 1, fpw); 
	fwrite(&(newpng->p_IDAT->type[0]), sizeof(newpng->p_IDAT->type), 1, fpw);
	fwrite(newpng->p_IDAT->p_data, newpng->p_IDAT->length, 1, fpw);
	fwrite(&(newpng->p_IDAT->crc), sizeof(newpng->p_IDAT->crc), 1, fpw);
	//write IEND
	fwrite(&(newpng->p_IEND->length), sizeof(newpng->p_IEND->length), 1, fpw); 
	fwrite(&(newpng->p_IEND->type[0]), sizeof(newpng->p_IEND->type), 1, fpw);
	fwrite(newpng->p_IEND->p_data, newpng->p_IEND->length, 1, fpw);
	fwrite(&(newpng->p_IEND->crc), sizeof(newpng->p_IEND->crc), 1, fpw);
	fclose(fpw);
	printf("%d\n", is_png("./all.png"));

	for (int i=0; i<(argc-1); i++){  //loop to free memory
   		free(calcs[i]);
   		free(imgs[i]->p_IHDR->p_data);
   		free(imgs[i]->p_IDAT->p_data);
   		free(imgs[i]->p_IEND->p_data);
   		free(imgs[i]);
   	}
   	return 0;
}
