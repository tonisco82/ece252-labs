/**
 * @brief: PNG related utility functions for ECE 252
 * Written by Devon Miller-Junk and Braden Bakker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strings
#include <arpa/inet.h> // For ntohl and htonl
#include "png.h" // For png structs and data
#include "crc/crc.c" // For crc generator function

#pragma once

/**
 * @brief: Generates and returns a valid CRC for a given chunk
 * @params:
 * data: pointer to a chunk struct. Must be filled with data.
 * @return: the CRC code
 */
unsigned long crc_generator(chunk_p data){
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

/**
 * @brief: Fills a chunk with data from the data pointer. Updates the offset.
 * @params:
 * out: allocated memory for the chunk struct (p_data should not be allocated). Will be filled by the function.
 * data: pointer to the beginning of the data to fill the chunk from.
 * offset: pointer to the current offset within the data. This value will be updated as the chunk is filled.
 * @note: de-allocating the memory where data and offset are stored is up to the user.
 * @return:
 * -1: Error in reading data
 * 0: got chunk
 */
int fill_chunk(chunk_p out, void* data, unsigned long *offset){
    //Read Length Field
    memcpy((void *)(&(out->length)), (void *)(data + *offset), CHUNK_LEN_SIZE);
    *offset += CHUNK_LEN_SIZE;
    out->length = ntohl(out->length);

    //Read Type Field
    memcpy((void *)(out->type), (void *)(data + *offset), CHUNK_TYPE_SIZE);
    *offset += CHUNK_TYPE_SIZE;

    //Read Data Field
    out->p_data = malloc(out->length);
    memcpy((void *)(out->p_data), (void *)(data + *offset), out->length);
    *offset += out->length;

    //Read CRC Field
    memcpy((void *)(&(out->crc)), (void *)(data + *offset), CHUNK_CRC_SIZE);
    *offset += CHUNK_CRC_SIZE;
    out->crc = ntohl(out->crc);

    return 0;
}

/**
 * @brief: Fills the target_png with the data from the data pointer.
 * @params:
 * target_png: pointer to a simple_png struct that is already allocated in memory (none of the chunks inside it are allocated). It will be filled with data.
 * data: pointer to the memory containing all the data for a valid PNG file.
 * @note: de-allocating the memory where data is stored is up to the user.
 * @return:
 * -1: Error in reading data
 * 0: Success
 */
int fill_png_struct(simple_PNG_p target_png, void* data){

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
    get_chunk_status = fill_chunk(target_png->p_IHDR, data, &offset);
    if(get_chunk_status != 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IDAT, data, &offset);
    if(get_chunk_status != 0) return -1;

    get_chunk_status = fill_chunk(target_png->p_IEND, data, &offset);
    if(get_chunk_status != 0) return -1;

    return 0;
}

/**
 * @brief: Transfers data inside of a chunk to memory, in the format required for a png.
 * @params:
 * target: a pointer to a pointer. Target will be made to point to the where the data will be stored. This function will allocate the memory.
 * size: pointer to a size variable. Size will be set to the number of bytes pointed to by *target.
 * chunk: pointer to the chunk data struct where the data is to be taken from.
 * @note: de-allocating the memory where chunk is stored is up to the user.
 * @return:
 * -1: Error
 * 0: Success
 */
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

    U32 chunk_crc = ntohl(chunk->crc);
    memcpy((void *)(*target + offset), (void *)(&(chunk_crc)), sizeof(U32));
    offset += sizeof(U32);

    return 0;
}

/**
 * @brief: Transfers data inside a simple_png to memory, in the format required for a png.
 * @params:
 * data: pointer to a pointer. Data will point to a pointer to where the data is stored. The actual data will be allocated by this function.
 * size: pointer to a size variable. Size will be set to the number of bytes of data pointed to by *data.
 * png_data: pointer to the simple_png struct to take the data from.
 * @note: de-allocating the memory where png_data is stored is up to the user.
 * @return:
 * -1: Error in transferring data
 * 0: success in transfering data
 */
int fill_png_data(void** data, unsigned long* size, simple_PNG_p png_data){

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


/**
 * @brief: Transforms a IHDR data struct into a data chunk struct
 * @params:
 * destination: a pointer to a chunk struct already allocated in memory (p_data is not allocated). Will be filled with data from the IHDR struct.
 * data: pointer to the IHDR struct with the data to use in filling the destination.
 * @note: de-allocating the memory where data is stored is up to the user.
 * @return:
 * -1: Error
 * 0: Success
 */
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

/**
 * @brief: Fills an IHDR struct with data from and IHDR 
 * @params:
 * out: pointer to the IHDR struct to be filled by the data. Should already be allocated.
 * data: pointer to the chunk struct with the data to be used to fill the IHDR struct.
 * @note: de-allocating the memory where data is stored is up to the user.
 * @return:
 * -1: Error
 * 0: Success
 */
int fill_IHDR_data(data_IHDR_p out, chunk_p data){
    if(data->type[0] != (char) 'I' || data->type[1] != (char) 'H' || data->type[2] != (char) 'D' || data->type[3] != (char) 'R') return -1;

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

/**
 * @brief: Checks the PNG header to ensure it is valid
 * @param: pointer to a PNG struct containing valid information
 * @return:
 * 0: invalid PNG header
 * 1: valid PNG header
 */
int png_header_checker(simple_PNG_p png_data){
    /** Check for mismatch in array **/
    for(int i = 0;i<PNG_HDR_SIZE;i++){
        if(png_header[i] != png_data->png_hdr[i]) return 0;
    }
    return 1;
}

/**
 * @brief: Checks the PNG struct to ensure it is a valid PNG
 * @param: pointer to a PNG struct containing valid information
 * @return:
 * 0: valid png
 * 1: invalid header
 * 2: invalid IHDR CRC
 * 3: invalid IDAT CRC
 * 4: invalid IEND CRC
 * 5: invalid IHDR chunk length
 * 6: invalid IEND chunk length
 */
int is_png_file(simple_PNG_p png_data){
    /** Check Header **/
    if(png_header_checker(png_data) == 0) return 1;
    /** Check CRC codes **/
    if(crc_generator(png_data->p_IHDR) != png_data->p_IHDR->crc) return 2;
    if(crc_generator(png_data->p_IDAT) != png_data->p_IDAT->crc) return 3;
    if(crc_generator(png_data->p_IEND) != png_data->p_IEND->crc) return 4;
    if(png_data->p_IHDR->length != DATA_IHDR_SIZE) return 5;
    if(png_data->p_IEND->length != 0) return 6;
    return 0;
}

/**
 * @brief: De-allocates the memory of a data-IHDR struct
 * This includes deallocating the data_IHDR struct itself
 */
void free_data_IHDR(data_IHDR_p data){
    free(data);
}

/**
 * @brief: De-allocates the memory of a chunk struct
 * This includes deallocating the chunk struct itself
 */
void free_chunk(chunk_p data){
    free(data->p_data);
    free(data);
}

/**
 * @brief: De-allocates the memory of a simple_PNG struct
 * This includes deallocating the simple_PNG struct itself
 */
void free_simple_PNG(simple_PNG_p data){
    free_chunk(data->p_IHDR);
    free_chunk(data->p_IDAT);
    free_chunk(data->p_IEND);
    free(data);
}

/**
 * @brief: Combines 2 PNGs into a single PNG.
 * @params:
 * out: a pointer to the resulting PNG after combining the two PNG files. Should already by allocated (only the simple_PNG is allocated not the chunks inside it)
 * png1: a pointer to the first PNG (will be placed on top).
 * png2: a pointer to the second PNG (will be placed on the bottom).
 * @note: de-allocating the memory where png1 and png2 are stored is up to the user.
 * @return:
 * -1: Error
 * 0: Success
 */
int combine_pngs(simple_PNG_p out, simple_PNG_p png1, simple_PNG_p png2){
    /** Initial Setup **/
        /** Find IHDR Data **/
    int fill_IHDR_status = 0;
    data_IHDR_p png1_IHDR = (data_IHDR_p) malloc(sizeof(struct data_IHDR));
    data_IHDR_p png2_IHDR = (data_IHDR_p) malloc(sizeof(struct data_IHDR));

    fill_IHDR_status = fill_IHDR_data(png1_IHDR, png1->p_IHDR);
    if(fill_IHDR_status != 0){
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        return -1;
    }

    fill_IHDR_status = fill_IHDR_data(png2_IHDR, png2->p_IHDR);
    if(fill_IHDR_status != 0){
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        return -1;
    }

        /** Ensure Same Width **/
    if(png1_IHDR->width != png1_IHDR->width){
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        return -1;
    }

    /** Inflation Section **/
    U32 combined_height = png1_IHDR->height + png2_IHDR->height;
    U64 data_size = combined_height*((png1_IHDR->width*4)+1);
    U8* catbuf = (U8 *) malloc(data_size);
    U64 len_inf = 0;    /* uncompressed data length */
    U64 len_tot = 0;    /* running tally of total length */
    int ret = 0;        /* debug param */

        /** Inflate First PNG **/
    ret = mem_inf(catbuf+len_tot, &len_inf, png1->p_IDAT->p_data, png1->p_IDAT->length); //automatically concatenate inflated data to buffer
    
    if (ret !=0){
        /** Error in inflating data **/
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        free(catbuf);
        return -1;
    }

    len_tot += len_inf;  //keep running total for buffer offset on next data inflate
    len_inf = 0;
    ret = 0;

        /** Inflate Second PNG **/
    ret = mem_inf(catbuf+len_tot, &len_inf, png2->p_IDAT->p_data, png2->p_IDAT->length); //automatically concatenate inflated data to buffer
    
    if (ret !=0){
        /** Error in inflating data **/
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        free(catbuf);
        return -1;
    }

    ret = 0;
    /** Deflation Section **/
    U8* defbuf = (U8 *) malloc(data_size);

    ret = mem_def(defbuf, &len_inf, catbuf, data_size, Z_DEFAULT_COMPRESSION);
    if (ret !=0){
        /** Error in deflating data **/
        free_data_IHDR(png1_IHDR);
        free_data_IHDR(png2_IHDR);
        free(catbuf);
        free(defbuf);
        return -1;
    }

    /** Creating New PNG Struct **/
        // Copy Header
    memcpy((void *)out->png_hdr, (void *)png1->png_hdr, PNG_HDR_SIZE);

        // Copy IHDR Chunk
    out->p_IHDR = (chunk_p) malloc(sizeof(struct chunk));
    png1_IHDR->height = combined_height;
    int fill_IHDR_status = fill_IHDR_chunk(out->p_IHDR, png1_IHDR);

        // Copy IDAT Chunk
    out->p_IDAT = (chunk_p) malloc(sizeof(struct chunk));
    memcpy((void *) out->p_IDAT, (void *) png1->p_IDAT, sizeof(struct chunk)); //Fine for initial setup. Update values after.
    out->p_IDAT->length = (U32)len_inf;
    out->p_IDAT->p_data = defbuf;

        // Copy IEND Chunk
    out->p_IEND = (chunk_p) malloc(sizeof(struct chunk));
    memcpy((void *) out->p_IEND, (void *) png1->p_IEND, sizeof(struct chunk)); //Fine since there is no data


    /** Cleanup **/
    free_data_IHDR(png1_IHDR);
    free_data_IHDR(png2_IHDR);
    free(catbuf);
        //Note: don't deallocate defbuf since it is used in the out png.

    if(fill_IHDR_status != 0) return -1;
    return 0;
}