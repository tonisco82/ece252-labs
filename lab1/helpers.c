/**
 * @brief: Helper Functions for ECE 252 Lab Course
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "./starter/png_util/lab_png.h"
#include "./starter/png_util/crc.c"

#pragma once

// Returns 0 on valid CRC, otherwise what the value should be.
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

// What the 8-Byte Header of the PNG should look like in integer format
int png_header[8] = {-119, 80, 78, 71, 13, 10, 26, 10};

// Returns 1 if the file is a png, 0 if not, -1 on error
// file_path is the relative file path to the file in question
int is_png(char* file_path){

    int buffer_len = 8;
    int fread_status;
    char* buffer = malloc(buffer_len*sizeof(char));
    
    FILE *fp = fopen(file_path, "rb");

    if(!fp){
        // File doesn't exist
        fclose(fp);
        free(buffer);
        return -1;
    }

    fread_status = fread(buffer, buffer_len*sizeof(char), 1, fp);

    if(fread_status != 1) {
        // Error in reading the png file
        fclose(fp);
        free(buffer);
        return -1;
    }

    for(int i=0;i<buffer_len;i++){
        if(buffer[i] != png_header[i]){
            // Not a PNG
            fclose(fp);
            free(buffer);
            return 0;
        }
    }

    free(buffer);
    fclose(fp);
    return 1;
}

// Takes the IHDR chunk data (should be a pointer to 25bytes of data)
// Files the data pointed to by the width and height pointers with the width and height of the PNG
// For information about how the IHDR data is formatted see the Readme
int get_data_IHDR(char* IHDR_chunk, data_IHDR_p data){

    int initial_position = 0;

    data->width = ntohl(*((U32*)(IHDR_chunk + initial_position)));
    initial_position += sizeof(data->width);

    data->height = ntohl(*((U32*)(IHDR_chunk + initial_position)));
    initial_position += sizeof(data->height);

    data->bit_depth = *((U8*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->bit_depth);

    data->color_type = *((U8*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->color_type);

    data->compression = *((U8*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->compression);

    data->filter = *((U8*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->filter);

    data->interlace = *((U8*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->interlace);

    return 0;
}

//Prints a string in hex form
void printStringInHex(U8 *var, unsigned int length){
    for(int i=0;i<length;i++){
        printf("%02x", *(var + i));
    }
    printf("\n");
}

// Fill Data IHDR
int fill_data_IHDR(data_IHDR_p data, chunk_p destination){
    destination->length = DATA_IHDR_SIZE;

    destination->type[0] = (char) 'I';
    destination->type[1] = (char) 'H';
    destination->type[2] = (char) 'D';
    destination->type[3] = (char) 'R';

    destination->p_data = malloc(DATA_IHDR_SIZE);
    memset (destination->p_data,'\0',DATA_IHDR_SIZE);

    destination->crc = 0;

    int offset = 0;

    printf("%u height %u width before byte conversion\n", data->height, data->width);

    U32 width = htonl((unsigned int)data->width);
    U32 height = htonl((unsigned int)data->height);

    printf("%lu height %lu width going into ihdr\n", height, width);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = width;
    offset += sizeof(U32);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = height;
    offset += sizeof(U32);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = data->bit_depth;
    offset += sizeof(U8);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = data->color_type;
    offset += sizeof(U8);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = data->compression;
    offset += sizeof(U8);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = data->filter;
    offset += sizeof(U8);

    printStringInHex(destination->p_data, 13);

    *(destination->p_data + offset) = data->interlace;
    offset += sizeof(data->interlace);

    printStringInHex(destination->p_data, 13);

    destination->crc = crccheck(destination);

    return 0;
}

// Takes a file path to a png, and an offset inside the file.
// Returns a chunk in the chunk data structure (out parameter)
// Return values:
// -1: Error in reading file
// 0: got chunk
int get_chunk(chunk_p out, char* file_path, long *offset){

    int fread_status = 0;

    // Open file at location
    FILE *fp = fopen(file_path, "rb");

    if(!fp){
        // File doesn't exist
        fclose(fp);
        return -1;
    }

    fseek(fp, *offset, SEEK_SET); 
    //Note: SEEK_SET is from beginning of file
    // SEEK_CUR is from current position, SEEK_END is from end

    // Read 4-Byte Length field
    unsigned int len = 0;
    fread_status = fread(&len, CHUNK_LEN_SIZE, 1, fp); //Read 4 more bytes

    if(fread_status != 1) {
        fclose(fp);
        return -1;
    }

    // Reverse Byte order and place into the length field
    len = ntohl(len);
    out->length = len;


    // Place Type
    fread_status = fread(&(out->type[0]), CHUNK_TYPE_SIZE, 1, fp);
    if(fread_status != 1) {
        fclose(fp);
        return -1;
    }


    // Allocate Memory for data and read
    U8 *data_temp = (U8 *)malloc(len);
    out->p_data = data_temp;
    if(len > 0) {
        fread_status = fread(data_temp, len, 1, fp);
        if(fread_status != 1) {
            fclose(fp);
            return -1;
        }
    }

    // CRC Field
    int chunkcrc;
    fread_status = fread(&chunkcrc, CHUNK_CRC_SIZE, 1, fp);
    if(fread_status != 1) {
        fclose(fp);
        return -1;
    }

    chunkcrc = ntohl(chunkcrc);
    out->crc = chunkcrc;

    *offset = ftell(fp);

    fclose(fp);
    return 0; //returns current file position on a success
}

// Takes a complete png file and returns all the data decoded into a simple_PNG data structure
// Return values:
// -1: Error in reading file
// 0: got png
// 1: Header does not match png header
int get_png(char* file_path, struct simple_PNG* png) {

    if(is_png(file_path) != 1){
        return 1;
    }
    
    chunk_p IHDR = (chunk_p) malloc(sizeof(struct chunk));
    chunk_p IDAT = (chunk_p) malloc(sizeof(struct chunk));
    chunk_p IEND = (chunk_p) malloc(sizeof(struct chunk));

    long offset = 8; // To avoid the initial 8 bytes from the png header
    int result = 0;

    result = get_chunk(IHDR, file_path, &offset);
    if(result != 0){

        free(IHDR->p_data);
        free(IHDR);
        free(IDAT);
        free(IEND);
        return result;
    }

    if(IHDR->length != 13){
        // Not a PNG since the header isn't 13 bytes long.
        free(IHDR->p_data);
        free(IHDR);
        free(IDAT);
        free(IEND);
        return 1;
    }

    result = get_chunk(IDAT, file_path, &offset);
    if(result != 0){

        free(IHDR->p_data);
        free(IHDR);
        free(IDAT->p_data);
        free(IDAT);
        free(IEND);
        return result;
    }

    result = get_chunk(IEND, file_path, &offset);
    if(result != 0){

        free(IHDR->p_data);
        free(IHDR);
        free(IDAT->p_data);
        free(IDAT);
        free(IEND->p_data);
        free(IEND);
        return result;
    }

    png->p_IHDR = IHDR;
    png->p_IDAT = IDAT;
    png->p_IEND = IEND;

    return result;
}


int write_png_file (char* file_path, struct simple_PNG* newpng){
    FILE *fpw;
	fpw = fopen (file_path, "wb+");

    if(!fpw){
        // File doesn't exist
        fclose(fpw);
        return -1;
    }

	//write png header
	for (int i=0; i<8; i++){
		fwrite(&png_header[i], 1, 1, fpw);  //this works for sure
	}

    unsigned int len = 0;
    unsigned long crc = 0;

	//write IHDR
    len = htonl(newpng->p_IHDR->length);
    crc = crccheck(newpng->p_IHDR);
    if(crc == 0){
        crc = newpng->p_IHDR->crc;
    }
    crc = htonl(crc);
	fwrite(&(len), CHUNK_LEN_SIZE, 1, fpw);
	fwrite(&(newpng->p_IHDR->type[0]), CHUNK_TYPE_SIZE, 1, fpw);
	fwrite(newpng->p_IHDR->p_data, newpng->p_IHDR->length, 1, fpw);
	fwrite(&(crc), CHUNK_CRC_SIZE, 1, fpw);

	//write IDAT
    len = htonl(newpng->p_IDAT->length);
    crc = crccheck(newpng->p_IDAT);
    if(crc == 0){
        crc = newpng->p_IDAT->crc;
    }
    crc = htonl(crc);
	fwrite(&(len), CHUNK_LEN_SIZE, 1, fpw);
	fwrite(&(newpng->p_IDAT->type[0]), CHUNK_TYPE_SIZE, 1, fpw);
	fwrite(newpng->p_IDAT->p_data, newpng->p_IDAT->length, 1, fpw);
	fwrite(&(crc), CHUNK_CRC_SIZE, 1, fpw);

	//write IEND
    len = htonl(newpng->p_IEND->length);
    crc = crccheck(newpng->p_IEND);
    if(crc == 0){
        crc = newpng->p_IEND->crc;
    }
    crc = htonl(crc);
	fwrite(&(len), CHUNK_LEN_SIZE, 1, fpw);
	fwrite(&(newpng->p_IEND->type[0]), CHUNK_TYPE_SIZE, 1, fpw);
	fwrite(newpng->p_IEND->p_data, newpng->p_IEND->length, 1, fpw);
	fwrite(&(crc), CHUNK_CRC_SIZE, 1, fpw);
	
	fclose(fpw);

    return 0;
}

int free_png(struct simple_PNG* png_file) {
    free(png_file->p_IHDR->p_data);
    free(png_file->p_IHDR);
    free(png_file->p_IDAT->p_data);
    free(png_file->p_IDAT);
    free(png_file->p_IEND->p_data);
    free(png_file->p_IEND);
    free(png_file);
    return 0;
}

// int main(int argc, char *argv[]) {

//     simple_PNG_p png_file = (simple_PNG_p) malloc(sizeof(struct simple_PNG));

//     int get_png_status = get_png(argv[1], png_file);

//     // Check for Error Cases
// 	if (get_png_status != 0) {
// 		printf("%s: Not a PNG file\n", argv[1]);
//         free(png_file);
//     	return get_png_status;
// 	}

//     printf("Finished png %d\n", write_png_file("all.png", png_file));


//     // Info was fine, and thus still has memory allocated
//     free(png_file->p_IHDR->p_data);
//     free(png_file->p_IHDR);
//     free(png_file->p_IDAT->p_data);
//     free(png_file->p_IDAT);
//     free(png_file->p_IEND->p_data);
//     free(png_file->p_IEND);

// 	// Deallocate Memory
// 	free(png_file);

// 	return 0;

// }