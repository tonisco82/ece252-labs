/**
 * @brief: Given a directory of PNG files, (each PNG file is a horizontal strip of 
 *         the same width, but differing heights, that form a larger image) concatenate 
 *         all the PNG images into one PNG image. The files are named according to the 
 *         convention *_N.png where N is a series of consecutive increasing numbers 
 *         (0, 1, 2, 3, 4, ...), which indicate the position of the image from top to bottom.
 *         The resulting combined PNG should be called all.png.
 * EXAMPLE: ./catpng ./img1.png ./png/img2.png
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "helpers.c"
#include "./png_util/crc.c"
#include "./png_util/zutil.c"


int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage example: ./catpng ./img1.png ./png/img2.png\n");
        return -1;
    }

	// **Fill Initial Array of PNG Structs**
	simple_PNG_p imgs[argc-1];

   	int get_png_status = 1;

   	for (int i=0; i<(argc-1); i++){
		// Allocate memory for PNG struct, fill it with the data
		imgs[i] = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
   		get_png_status = get_png(argv[i+1], imgs[i]);

		
   		if (get_png_status != 0) {
			// PNG struct creation failed
			printf("%s: failed to get png\n", argv[i+1]);

			for(int j=0;j<i;j++){
				free_png(imgs[j]);
			}
			free(imgs[i]);

	    	return get_png_status;
		} 
   	}

	// **Fill Initial Array of Header Data**

   	U32 height = 0;
   	data_IHDR_p calcs[argc-1];

   	for (int i=0; i<(argc-1); i++){  //loop get IHDR data for dimensions
	   	calcs[i] = (data_IHDR_p)malloc(sizeof(struct data_IHDR));
   		int get_data_IHDR_status = get_data_IHDR((char*) imgs[i]->p_IHDR->p_data, calcs[i]);

		if (get_data_IHDR_status != 0) {
			printf("%s: failed to get IHDR data\n", argv[i+1]);
			for(int j=0;j<=i;j++){
				free(calcs[j]);
			}
			for(int j=0;j<=(argc-1);j++){
				free_png(imgs[j]);
			}
	    	return get_png_status;
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

   	for (int i=0; i<(argc-1); i++){ //loop inflate data
   		len_def = imgs[i]->p_IDAT->length;
   		len_inf = 0;
   		ret = mem_inf(catbuf+len_tot, &len_inf, imgs[i]->p_IDAT->p_data, len_def); //automatically concatenate inflated data to buffer
	    
		if (ret !=0){
	        printf("StdError: mem_def failed. ret = %d.\n", ret);
			for(int j=0;j<=(argc-1);j++){
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
		for(int j=0;j<=(argc-1);j++){
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

	for (int i=0; i<(argc-1); i++){  //loop to free memory
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
