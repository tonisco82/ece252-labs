/**
 * @brief: Prints the dimensions of a valid PNG file or prints an error message. 
 *          If the input file is a PNG, but contains CRC errors, use the same 
 *          output as the pngcheck command.
 * EXAMPLE: ./pnginfo WEEF_1.png
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "helpers.c"
#include "crc.h"
#include "lab_png.h"

int crccheck (struct chunk* data){
	int crcflag = 0;
    ntol(data->p_data);
	int testcrc = crc(data->p_data, data->length)
	if (testcrc != data->crc){
		crcflag = testcrc;
	}
	return crcflag;  //returns 0 if pass, returns crc found if fail
}

int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }

    FILE *fp;
    fp = argv[1];

    if (!ispng(fp)){
    	printf("%s: Not a PNG file\n", argv[1]);
    	return 0;
    } else {
    	int png = 1;
    	data_IHDR_p out = malloc(sizeof(data_IHDR));
    	chunk_p IHDR = malloc(sizeof(chunk));
    	int offset = 8;
    	offset = get_chunk(IHDR, fp, &offset);
    	get_data_IHDR(IHDR->p_data, out);
    	if (out->width <= 0 || out->height <=0){
    		png=0;
    	} else if (out->bit_depth != 8 || out->color_type != 6) {
    		png=0;
    	} else if (out->compression !=0 || out->filter !=0 || out->interlace !=0 ||){
    		png=0;
    	}
    	if (png == 0) {
    		printf("%s: Not a PNG file\n", argv[1]);
    		return 0;
    	} else {
    		int x = crccheck(IHDR);
    		if (crcflag !=0){
    			printf("%s: %d x %d\n", argv[1], out->width, out->height);
    			printf("IHDR chunk CRC error: computed %d, expected %d\n", crcflag, data->crc);
                free(out);
                free(IHDR);               
    			return 0;
    		}
    		chunk_p data = malloc(sizeof(chunk);
    		get_chunk(data, fp, &offset);
    		x = crccheck(data);
    		if (crcflag !=0){
    			printf("%s: %d x %d\n", argv[1], out->width, out->height);
    			printf("IDAT chunk CRC error: computed %d, expected %d\n", crcflag, data->crc);
                free(out);
                free(IHDR);
                free(data->p_data)
                free(data);
    			return 0;
    		}
    		chunk_p IEND  = malloc(sizeof(chunk);
    		get_chunk(IEND, fp, &offset);
    		x = crccheck(IEND);
    		if (crcflag !=0){
    			printf("%s: %d x %d\n", argv[1], out->width, out->height);
    			printf("IEND chunk CRC error: computed %d, expected %d\n", crcflag, data->crc);
                free(out);
                free(IHDR);
                free(data->p_data)
                free(data);
                free(IEND);
    			return 0;
    		}
            free(out);
            free(IHDR);
            free(data->p_data)
            free(data);
            free(IEND);
    			printf("%s: %d x %d\n", argv[1], out->width, out->height);
    	}
    }
    return 0;
}