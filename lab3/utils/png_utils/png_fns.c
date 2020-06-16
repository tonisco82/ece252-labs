/**
 * @brief: PNG related utility functions for ECE 252
 * Written by Devon Miller-Junk and Braden Bakker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lab_png.h"
#include "crc.c"

#pragma once

//Separate out chunks

//Chunks to single pointer

//IHDR Chunk to data_IHDR

//data_IHDR to IHDR Chunk

//Generates and returns a valid CRC for a given chunk
//Returns the new crc
unsigned long crc_generator(struct chunk* data){
	// Length of the crc buffer
	int len = data->length + CHUNK_TYPE_SIZE; // Length of the data plus the length of the type field

	// Declare crc buffer
	unsigned char * restrict chunk_crc_data = (unsigned char *)malloc(len);

	// Copy type field into buffer
	memcpy((void *)chunk_crc_data, data->type, CHUNK_TYPE_SIZE);

	// Copy data field into buffer
    memcpy((void *)(chunk_crc_data + CHUNK_TYPE_SIZE), data->p_data, data->length);

	// Run crc comparison
	unsigned long crc_code = crc(chunk_crc_data, len);

    free(chunk_crc_data);

    return crc_code;
}

//Header Checker
// Return values:
// -1: Error in reading data
// 0: invalid png header
// 1: valid png header
int png_header_checker(simple_PNG_p png_data){

}

//Is a png file checker