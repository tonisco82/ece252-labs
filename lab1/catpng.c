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

   	int height;

   	for (int i=0; i<(argc-1); i++){  //loop get IHDR data for dimensions
   		//get_IHDR(imgs[i]->p_data);
   	}

   	
   	for (int i=0; i<(argc-1); i++){ //loop inflate data
   		void*a = malloc()
   		if (0 != mem_inf()
   	}
   	//concatenate and deflate data

   	//write to new file

   	return 0;
}
