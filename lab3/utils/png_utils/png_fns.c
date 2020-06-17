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

// Takes a point to the png data, and an offset inside the data.
// Returns a chunk in the chunk data structure (out parameter)
// Updates the offset.
// out should already be allocated in memory
// Return values:
// -1: Error in reading data
// 0: got chunk
int fill_chunk(chunk_p out, void* data, unsigned long *offset){
    //Read Length Field
    memcpy((void *)(&(out->length)), (data + *offset), CHUNK_LEN_SIZE);
    *offset += CHUNK_LEN_SIZE;

    out->length = ntohl(out->length);

    //Read Type Field
    memcpy((void *)(out->type), (data + *offset), CHUNK_TYPE_SIZE);
    *offset += CHUNK_TYPE_SIZE;

    //Read Data Field
    memcpy((void *)(out->p_data), (data + *offset), out->length);
    *offset += out->length;

    //Read CRC Field
    memcpy((void *)(&(out->crc)), (data + *offset), CHUNK_CRC_SIZE);
    *offset += CHUNK_CRC_SIZE;

    return 0;
}

//Separate out chunks
//target_png is a pointer to the png to fill, none of the data should be allocated yet
//data is a pointer to the png file data
//length is the length of the png file data in bytes
// Return values:
// -1: Error in reading data
// 0: got png
int fill_png(simple_PNG_p target_png, void* data){

    unsigned long offset = 0; //Current offset in the data pointer
    int get_chunk_status = 0;

    // Copy the header
    memcpy((void *)target_png->png_hdr, data, PNG_HDR_SIZE * sizeof(U8));
    offset += PNG_HDR_SIZE * sizeof(U8);

    target_png->p_IHDR = malloc(sizeof(struct chunk));
    target_png->p_IDAT = malloc(sizeof(struct chunk));
    target_png->p_IEND = malloc(sizeof(struct chunk));

    get_chunk_status = fill_chunk(target_png->p_IHDR, &offset);
    if(get_chunk_status !== 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IDAT, &offset);
    if(get_chunk_status !== 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IEND, &offset);
    if(get_chunk_status !== 0) return -1;

    return 0;
}

// Fills memory with data from a chunk struct
// Does not de-allocate the chunk
// Return values:
// -1: Error
// 0: success
int chunk_to_data(void **target, chunk_p chunk){
}

//png struct to single pointer
//Takes a simple_PNG data struct, and converts it into a png file in memory (in network order)
//png_data will not be de-allocated.
// Return values:
// -1: Error in transferring data
// 0: success in transfering data
int fill_png(void** data, simple_PNG_p png_data){
}

//Transforms a IHDR data struct into a data chunk struct
//chunk_p must already by allocated in memory, but p_data must not be allocated.
// Return values:
// -1: Error
// 0: Success
int fill_IHDR_chunk(chunk_p destination, data_IHDR_p data){

    destination->length = DATA_IHDR_SIZE;

    destination->type[0] = (char) 'I';
    destination->type[1] = (char) 'H';
    destination->type[2] = (char) 'D';
    destination->type[3] = (char) 'R';

    destination->p_data = malloc(DATA_IHDR_SIZE);

    destination->crc = 0;

    int offset = 0;

    U32 width = htonl((unsigned int)data->width);
    U32 height = htonl((unsigned int)data->height);

    U32 *temp = (U32 *)destination->p_data;

    *temp = width;
    offset += sizeof(U32);

    temp = (U32 *)(destination->p_data + offset);

    *temp = height;
    offset += sizeof(U32);

    *(destination->p_data + offset) = data->bit_depth;
    offset += sizeof(U8);

    *(destination->p_data + offset) = data->color_type;
    offset += sizeof(U8);

    *(destination->p_data + offset) = data->compression;
    offset += sizeof(U8);

    *(destination->p_data + offset) = data->filter;
    offset += sizeof(U8);

    *(destination->p_data + offset) = data->interlace;
    offset += sizeof(data->interlace);

    destination->crc = crc_generator(destination);

    return 0;
}

//IHDR Chunk to IHDR_data struct
// Have out already allocated, this will just fill it
// Return values:
// -1: Error
// 0: Success
int fill_IHDR_data(data_IHDR_p out, chunk_p data){
}

//Header Checker
// Return values:
// 0: invalid png header
// 1: valid png header
int png_header_checker(simple_PNG_p png_data){
    for(int i = 0;i<PNG_HDR_SIZE;i++){
        if(png_header[i] !== png_data->png_hdr[i]) return 0;
    }
    return 1;
}

//Is a png file checker
// Return values:
// 0: invalid png
// 1: valid png
int is_png_file(simple_PNG_p png_data){
    if(png_header_checker(png_data) == 0) return 0;
    if(crc_generator(png_data->p_IHDR) !== png_data->p_IHDR->crc) return 0;
    if(crc_generator(png_data->p_IDAT) !== png_data->p_IDAT->crc) return 0;
    if(crc_generator(png_data->p_IEND) !== png_data->p_IEND->crc) return 0;
    return 1;
}