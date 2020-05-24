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
int get_png_info(char* file_path, struct simple_PNG* png_file) {

	unsigned long crcflag = 0;

	// Decode PNG file into struct
	int get_png_status = get_png(file_path, png_file);

	// Check for Error Cases
	if (get_png_status != 0) {
		printf("%s: Not a PNG file\n", file_path);

    	return get_png_status;
	}

	// **CRC Check for IHDR**
	crcflag = crccheck(png_file->p_IHDR);
	if (crcflag !=0){
		printf("%s: %d x %d\n", file_path, png_file->p_IHDR->width, png_file->p_IHDR->height);
		printf("IHDR chunk CRC error: computed %d, expected %d\n", crcflag, png_file->p_IHDR->crc);

		return 2;
	}

	// **CRC Check for IDAT**
	crcflag = crccheck(png_file->p_IDAT);
	if (crcflag !=0){
		printf("%s: %d x %d\n", file_path, png_file->p_IHDR->width, png_file->p_IHDR->height);
		printf("IDAT chunk CRC error: computed %d, expected %d\n", crcflag, png_file->p_IDAT->crc);

		return 2;
	}

	// **CRC Check for IEND**
	crcflag = crccheck(png_file->p_IEND);
	if (crcflag !=0){
		printf("%s: %d x %d\n", file_path, png_file->p_IHDR->width, png_file->p_IHDR->height);
		printf("IEND chunk CRC error: computed %d, expected %d\n", crcflag, png_file->p_IEND->crc);

		return 2;
	}

	// PNG looks fine

	printf("%s: %d x %d\n", file_path, png_file->p_IHDR->width, png_file->p_IHDR->height);

	return 0;
}

int main(int argc, char *argv[]) {
	
    if(argc != 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }
	
	simple_PNG_p png_file = (simple_PNG_p) malloc(sizeof(simple_PNG));

    int png_info_status = get_png_info(argv[1], png_file);

	if (png_info_status == 0 || png_info_status == 2){

		// Info was fine, and thus still has memory allocated

		free(png_file->p_IHDR->p_data);
		free(png_file->p_IHDR);
		free(png_file->p_IDAT->p_data);
		free(png_file->p_IDAT);
		free(png_file->p_IEND->p_data);
		free(png_file->p_IEND);


	}

	free(png_file);

	return 0;
}
