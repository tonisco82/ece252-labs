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
#include "crc.h"
#include "lab_png.h"
#include "zutil.h"

int crcgen (struct chunk* data){
	U32 crcflag = 0;
    ntohl(data->p_data);
	U32 testcrc = crc(data->p_data, data->length)
	if (testcrc != data->crc){
		crcflag = testcrc;
	}
	return crcflag;  //returns 0 if pass, returns crc found if fail
}

int main(int argc, char *argv[]) {

    if(argc <= 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }

    FILE *fp;
    fp = argv[1];
   	simple_PNG_p imgs[argc-1];

   	for (int i=0; i<(argc-1); i++){
   		fp = argv[i+1];
   		get_png(fp, imgs[i]); 
   	}

   	int height = 0;
   	int width = 0;
   	data_IHDR_p calcs[argc-1];

   	for (int i=0; i<(argc-1); i++){  //loop get IHDR data for dimensions
   		get_data_IHDR(imgs[i]->p_IHDR, calcs[i]);
   		height += calcs[i]->height;
   	}
   	width = calcs[0]->width;

	void* catbuf = malloc(height*((width*4)+1));
	U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    U64 len_tot = 0;      //running tally of total length
    int ret = 0;        //debug param

   	for (int i=0; i<(argc-1); i++){ //loop inflate data
   		len_def = imgs[i]->p_IDAT->length;
   		len_inf = 0;
   		ret = mem_inf(catbuf+len_tot, &len_inf, imgs[i]->IDAT->p_data, len_def); //automatically concatenate inflated data to buffer
	    if (ret !=0){
	        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
	        return ret;
	    }
	    len_tot += len_inf;  //keep running total for buffer offset on next data inflate
    }

    void* newdata = malloc(height*((width*4)+1)); //new data buffer

    //deflate data to new data buffer
    ret = mem_def(newdata &len_def, catbuf, (height*((width*4)+1)), Z_DEFAULT_COMPRESSION);
    if (ret !=0){
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }
    // now we should have the proper compressed IDAT data
    //from now on use initial png to create new one
    U32 newheight = height;

    newpng = imgs[0];
    newpng->IHDR->p_data+sizeof(U32) = htonl(newheight); //directly replace height in data
    newpng->IDAT->p_data = newdata;
    newpng->IDAT->length = len_def;
    newpng->IDAT->crc = crcgen(newpng->IDAT);
    newpng->IHDR->crc = crcgen(newpng->IHDR);

    //we should now have newpng with all of the data we need
    //now adjust data for easy writing
    htonl(newpng->IDAT->length);
    htonl(newpng->IDAT->crc);
    htonl(newpng->IHDR->length);
    htonl(newpng->IHDR->crc);

   	FILE *fpw;
	fpw = fopen ("all.png", "wb+");
	//write png header
	int png_header[8] = {-119, 80, 78, 71, 13, 10, 26, 10};
	fwrite(&png_header[0], sizeof(png_header), 1, fpw);
	//write IHDR
	fwrite(&(newpng->IHDR->length), sizeof(newpng->IHDR->length), 1, fpw); 
	fwrite(&(newpng->IHDR->type[0]), sizeof(newpng->IHDR->type), 1, fpw);
	fwrite(newpng->IHDR->p_data, newpng->IHDR->length, 1, fpw);
	fwrite(&(newpng->IHDR->crc), sizeof(newpng->IHDR->crc), 1, fpw);
	//write IDAT
	fwrite(&(newpng->IDAT->length), sizeof(newpng->IDAT->length), 1, fpw); 
	fwrite(&(newpng->IDAT->type[0]), sizeof(newpng->IDAT->type), 1, fpw);
	fwrite(newpng->IDAT->p_data, newpng->IDAT->length, 1, fpw);
	fwrite(&(newpng->IDAT->crc), sizeof(newpng->IDAT->crc), 1, fpw);
	//write IEND
	fwrite(&(newpng->IEND->length), sizeof(newpng->IEND->length), 1, fpw); 
	fwrite(&(newpng->IEND->type[0]), sizeof(newpng->IEND->type), 1, fpw);
	fwrite(newpng->IEND->p_data, newpng->END->length, 1, fpw);
	fwrite(&(newpng->IEND->crc), sizeof(newpng->IEND->crc), 1, fpw);

	for (int i=0; i<(argc-1); i++){  //loop to free memory
   		free(calcs[i]);
   		free(imgs[i]->IHDR->p_data);
   		free(imgs[i]->IDAT->p_data);
   		free(imgs[i]->IEND->p_data);
   		free(imgs[i]);
   	}
   	return 0;
}
