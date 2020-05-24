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
#include "./starter/png_util/crc.h"

unsigned long crccheck (struct chunk* data){
    ntolh(data->p_data);
	unsigned long testcrc = crc(data->p_data, data->length)
	if (testcrc != data->crc){
		return  testcrc;
	}
	return (unsigned long) 0;  //returns 0 if pass, returns crc found if fail
}


// Checks if the file is a png
// Return Code:
// 0: Is a PNG
// -1: Error
// 1: Not a PNG
// 2: CRC Error
int get_png_info(char* file_path) {

	// Check to make sure the file is a png
    if (!is_png(file_path)){

    	printf("%s: Not a PNG file\n", file_path);
    	return 1;

    }

	// Getting the png info
	data_IHDR_p IHDR_data = malloc(sizeof(struct data_IHDR));
	chunk_p IHDR = malloc(sizeof(struct chunk));

	int offset = 8; // Initial offset to avoid first 8 bytes of png header data
	int get_chunk_status = 0;

	// **Check the CRC of the IHDR chunk**

	get_chunk_status = get_chunk(IHDR, file_path, &offset); //Get IHDR data pointer
	if(get_chunk_status != 0){
		printf("%s: Not a PNG file\n", file_path);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);

    	return 1;
	}
	get_data_IHDR(IHDR->p_data, IHDR_data); //Get IHDR specific data

	// Check to make sure the PNG is valid

	// NOTE: The following is commented out because a PNG file could not meet these specifications
	// For this lab, all png files should meet these specifications, but not in general

	/*
	if (IHDR_data.width == 0 || IHDR_data.height ==0){
		printf("%s: Not a PNG file\n", argv[1]);
		return -1;
	} else if (IHDR_data.bit_depth != 8 || IHDR_data.color_type != 6) {
		printf("%s: Not a PNG file\n", argv[1]);
		return -1;
	} else if (IHDR_data.compression != 0 || IHDR_data.filter != 0 || IHDR_data.interlace != 0){
		printf("%s: Not a PNG file\n", argv[1]);
		return -1;
	}
	*/

	unsigned long crcflag = crccheck(IHDR);

	if (crcflag != 0){
		printf("%s: %d x %d\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IHDR chunk CRC error: computed %lu, expected %d\n", crcflag, data->crc);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);

		return 2;
	}

	// **Check the CRC of the IDAT chunk**

	chunk_p IDAT = malloc(sizeof(chunk);

	get_chunk_status = get_chunk(IDAT, fp, &offset);
	if(get_chunk_status != 0){
		printf("%s: Not a PNG file\n", file_path);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);
		free(IDAT->p_data);
		free(IDAT);

    	return 1;
	}

	crcflag = crccheck(IDAT);
	if (crcflag !=0){
		printf("%s: %d x %d\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IDAT chunk CRC error: computed %d, expected %d\n", crcflag, IDAT->crc);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);
		free(IDAT->p_data);
		free(IDAT);

		return 2;
	}

	// **Check the CRC of the IEND chunk**

	chunk_p IEND  = malloc(sizeof(chunk);

	get_chunk_status = get_chunk(IEND, fp, &offset);
	if(get_chunk_status != 0){
		printf("%s: Not a PNG file\n", file_path);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);
		free(IDAT->p_data);
		free(IDAT);
		free(IEND->p_data);
		free(IEND);

    	return 1;
	}

	crcflag = crccheck(IEND);
	if (crcflag !=0){
		printf("%s: %d x %d\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IEND chunk CRC error: computed %d, expected %d\n", crcflag, IEND->crc);

		free(IHDR_data);
		free(IHDR->p_data);
		free(IHDR);
		free(IDAT->p_data);
		free(IDAT);
		free(IEND->p_data);
		free(IEND);

		return 2;
	}

	printf("%s: %d x %d\n", file_path, IHDR_data->width, IHDR_data->height);

	free(IHDR_data);
	free(IHDR->p_data);
	free(IHDR);
	free(IDAT->p_data);
	free(IDAT);
	free(IEND->p_data);
	free(IEND);
	
    return 0;
}

int main(int argc, char *argv[]) {
	
    if(argc != 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }

    int png_info_status = get_png_info(argv[1]);
	return png_info_status;
}
