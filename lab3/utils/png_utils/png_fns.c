/**
 * @brief: PNG related utility functions for ECE 252
 * Written by Devon Miller-Junk and Braden Bakker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "png.h"
#include "crc/crc.c"

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
    memcpy((void *)(&(out->length)), (void *)(data + *offset), CHUNK_LEN_SIZE);
    *offset += CHUNK_LEN_SIZE;

    out->length = ntohl(out->length);

    //Read Type Field
    memcpy((void *)(out->type), (void *)(data + *offset), CHUNK_TYPE_SIZE);
    *offset += CHUNK_TYPE_SIZE;

    //Read Data Field
    memcpy((void *)(out->p_data), (void *)(data + *offset), out->length);
    *offset += out->length;

    //Read CRC Field
    memcpy((void *)(&(out->crc)), (void *)(data + *offset), CHUNK_CRC_SIZE);
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

    /** Allocate memory for the chunks **/
    target_png->p_IHDR = malloc(sizeof(struct chunk));
    target_png->p_IDAT = malloc(sizeof(struct chunk));
    target_png->p_IEND = malloc(sizeof(struct chunk));

    /** Fill the chunks with data **/
    get_chunk_status = fill_chunk(target_png->p_IHDR, &offset);
    if(get_chunk_status !== 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IDAT, &offset);
    if(get_chunk_status !== 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IEND, &offset);
    if(get_chunk_status !== 0) return -1;

    return 0;
}

// Allocated and fills memory with data from a chunk struct
// Does not de-allocate the chunk
// Sets size to the size of the target data;
// Return values:
// -1: Error
// 0: success
int chunk_to_data(void **target, unsigned long* size, chunk_p chunk){
    /** Determine size **/
    *size = CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE + CHUNK_CRC_SIZE + chunk->length;
    unsigned int offset = 0;

    /** Allocate memory **/
    *target = (void *) malloc(*size);

    /** Copy data over **/
    U32 chunk_data_len = ntohl(chunk->length);
    memcpy((void *)(*target + offset), (void *)(&(chunk_data_len)), sizeof(U32));
    offset += sizeof(U32);

    memcpy((void *)(*target + offset), chunk->type, CHUNK_TYPE_SIZE * sizeof(U8));
    offset += CHUNK_TYPE_SIZE * sizeof(U8);

    memcpy((void *)(*target + offset), chunk->p_data, chunk->length);
    offset += chunk->length;

    memcpy((void *)(*target + offset), (void *)(&(chunk->crc)), sizeof(U32));
    offset += sizeof(U32);

    return 0;
}

//png struct to single pointer
//Takes a simple_PNG data struct, and converts it into a png file in memory (in network order)
//png_data will not be de-allocated.
// Updates size to the size of the data pointer
// Return values:
// -1: Error in transferring data
// 0: success in transfering data
int fill_png(void** data, unsigned long* size, simple_PNG_p png_data){

    /** Move chunks to memory **/
    unsigned long size_IHDR = 0;
    void *data_IHDR;
    chunk_to_data(&data_IHDR, &size_IHDR, png_data->p_IHDR);

    unsigned long size_IDAT = 0;
    void *data_IDAT;
    chunk_to_data(&data_IDAT, &size_IDAT, png_data->p_IDAT);

    unsigned long size_IEND = 0;
    void *data_IEND;
    chunk_to_data(&data_IEND, &size_IEND, png_data->p_IEND);

    /** Allocate new memory **/

    *size = PNG_HDR_SIZE + size_IHDR + size_IDAT + size_IEND;

    *data = malloc(*size);

    unsigned int offset = 0;

    /** Copy data over **/

    memcpy((void *)(*data + offset), (void *)(png_data->png_hdr), PNG_HDR_SIZE);
    offset += PNG_HDR_SIZE;

    memcpy((void *)(*data + offset), data_IHDR, size_IHDR);
    offset += size_IHDR;

    memcpy((void *)(*data + offset), data_IDAT, size_IDAT);
    offset += size_IDAT;

    memcpy((void *)(*data + offset), data_IEND, size_IEND);
    offset += size_IEND;

    /** Clean-up **/
    free(data_IHDR);
    free(data_IDAT);
    free(data_IEND);

    return 0;
}

//Transforms a IHDR data struct into a data chunk struct
//chunk_p must already by allocated in memory, but p_data must not be allocated.
// Return values:
// -1: Error
// 0: Success
int fill_IHDR_chunk(chunk_p destination, data_IHDR_p data){

    /** Determine Size **/
    destination->length = (U32) DATA_IHDR_SIZE;

    /** Copy Data Over **/
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

    /** Generate CRC **/
    destination->crc = crc_generator(destination);

    return 0;
}

//IHDR Chunk to IHDR_data struct
// Have out already allocated, this will just fill it
// Return values:
// -1: Error
// 0: Success
int fill_IHDR_data(data_IHDR_p out, chunk_p data){

    unsigned int offset = 0;

    /** Copy Data Over **/
    memcpy((void *)(&(out->width)),(void *)(data->p_data + offset), sizeof(U32));
    offset += sizeof(U32);
    out->width = ntohl((unsigned int)out->width);

    memcpy((void *)(&(out->height)),(void *)(data->p_data + offset), sizeof(U32));
    offset += sizeof(U32);
    out->height = ntohl((unsigned int)out->height);

    memcpy((void *)(&(out->bit_depth)),(void *)(data->p_data + offset), sizeof(U8));
    offset += sizeof(U8);

    memcpy((void *)(&(out->color_type)),(void *)(data->p_data + offset), sizeof(U8));
    offset += sizeof(U8);

    memcpy((void *)(&(out->compression)),(void *)(data->p_data + offset), sizeof(U8));
    offset += sizeof(U8);

    memcpy((void *)(&(out->filter)),(void *)(data->p_data + offset), sizeof(U8));
    offset += sizeof(U8);

    memcpy((void *)(&(out->interlace)),(void *)(data->p_data + offset), sizeof(U8));
    offset += sizeof(U8);

    return 0;
}

//Header Checker
// Return values:
// 0: invalid png header
// 1: valid png header
int png_header_checker(simple_PNG_p png_data){
    /** Check for mismatch in array **/
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
    /** Check Header **/
    if(png_header_checker(png_data) == 0) return 0;
    /** Check CRC codes **/
    if(crc_generator(png_data->p_IHDR) !== png_data->p_IHDR->crc) return 0;
    if(crc_generator(png_data->p_IDAT) !== png_data->p_IDAT->crc) return 0;
    if(crc_generator(png_data->p_IEND) !== png_data->p_IEND->crc) return 0;
    return 1;
}

//De-allocates the memory of a data-IHDR struct
// This includes deallocating the data_IHDR struct itself
void free_data_IHDR(data_IHDR_p data){
    free(data);
}

//De-allocates the memory of a chunk struct
// This includes deallocating the chunk struct itself
void free_chunk(chunk_p data){
    free(data->p_data);
    free(data);
}

//De-allocates the memory of a simple_PNG struct
// This includes deallocating the simple_PNG struct itself
void free_simple_PNG(simple_PNG_p data){
    free_chunk(data->p_IHDR);
    free_chunk(data->p_IDAT);
    free_chunk(data->p_IEND);
    free(data);
}

