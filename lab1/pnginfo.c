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
#include "./starter/png_util/crc.c"

unsigned long crccheck (struct chunk* data){
    //ntohl(data->p_data);
	unsigned long testcrc = crc(data->p_data, data->length);
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

	// Format the Header Data
	data_IHDR_p IHDR_data = (data_IHDR_p)malloc(sizeof(struct data_IHDR));
	int get_data_IHDR_status = get_data_IHDR((char*) png_file->p_IHDR->p_data, IHDR_data);
	if (get_data_IHDR_status != 0) {
		printf("%s: Not a PNG file\n", file_path);
		free(IHDR_data);
    	return get_png_status;
	}

	// **CRC Check for IHDR**
	crcflag = crccheck(png_file->p_IHDR);
	if (crcflag !=0){
		printf("%s: %u x %u\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IHDR chunk CRC error: computed %lu, expected %u\n", crcflag, png_file->p_IHDR->crc);
		free(IHDR_data);
		return 2;
	}

	// **CRC Check for IDAT**
	crcflag = crccheck(png_file->p_IDAT);
	if (crcflag !=0){
		printf("%s: %u x %u\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IDAT chunk CRC error: computed %lu, expected %u\n", crcflag, png_file->p_IDAT->crc);
		free(IHDR_data);
		return 2;
	}

	// **CRC Check for IEND**
	crcflag = crccheck(png_file->p_IEND);
	if (crcflag !=0){
		printf("%s: %u x %u\n", file_path, IHDR_data->width, IHDR_data->height);
		printf("IEND chunk CRC error: computed %lu, expected %u\n", crcflag, png_file->p_IEND->crc);
		free(IHDR_data);
		return 2;
	}

	// PNG looks fine
	printf("%s: %u x %u\n", file_path, IHDR_data->width, IHDR_data->height);
	free(IHDR_data);
	return 0;
}

int main(int argc, char *argv[]) {
	
    if(argc != 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }
	
	//Declare png "object"
	simple_PNG_p png_file = (simple_PNG_p) malloc(sizeof(struct simple_PNG));
	
	// Format the png
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

	// Deallocate Memory
	free(png_file);

	return 0;
}
