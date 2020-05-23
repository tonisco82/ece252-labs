/**
 * @brief: Helper Functions for ECE 252 Lab Course
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "lab_png.h"

#pragma once

// What the 8-Byte Header of the PNG should look like in integer format
int png_header[8] = {-119, 80, 78, 71, 13, 10, 26, 10};

// Returns 1 if the file is a png, 0 if not, -1 on error
// file_path is the relative file path to the file in question
int is_png(char* file_path){

    int buffer_len = 8;
    int fread_status;
    
    FILE *fp;

    char buffer[buffer_len];

    fp = fopen(file_path, "rb");

    fread_status = fread(buffer, buffer_len, 1, fp);

    for(int i=0;i<buffer_len;i++){
        if(buffer[i] != png_header[i]){
            // Not a PNG

            fclose(fp);
            return 0;
        }
    }

    if(fread_status != 1) {
        printf("Error in reading the png file");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 1;
}

// Takes the IHDR chunk data (should be a pointer to 25bytes of data)
// Files the data pointed to by the width and height pointers with the width and height of the PNG
// For information about how the IHDR data is formatted see the Readme
int get_data_IHDR(char* IHDR_chunk, struct data_IHDR* data){

    int initial_position = 8;

    data->width = *((U32*)(IHDR_chunk + initial_position));
    initial_position += sizeof(data->width);

    data->height = *((U32*)(IHDR_chunk + initial_position));
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

int get_chunk(struct chunk *out, FILE *fp, long *offset){
    int buf_len = 256;
    int fread_status;
    
    FILE *fp;

    fp = fopen(file_path, "rb");

    fseek(fp, *offset, SEEK_SET)
    int len;
    fread_status = fread(&len, CHUNK_LEN_SIZE, 1, fp);

    ntohl(len);
    out->length = len;

    void*a = malloc(len);

    fread(&(out->type[0]), CHUNK_TYPE_SIZE, 1, fp);
    fread(a, len, 1, fp)
    out->p_data = a;

    int chunkcrc;
    fread_status = fread(&chunkcrc, CHUNK_CRC_SIZE, 1, fp);
    ntohl(chunkcrc);
    out->crc = chunkcrc;

    if(fread_status != 1) {
        printf("Error in reading the png file");
        fclose(fp);
        return -1;
    }
    *offset = ftell(fp);
    fclose(fp);
    return 0; //returns current file position on a success
}

int get_png(FILE* fp, struct simple_PNG* png) {
    chunk_p IHDR;
    chunk_p IDAT;
    chunk_p IEND;
    int* offset = 8;
    int result = 0;
    result = get_chunk(IHDR, fp, &offset);
    if (result ==0){
        result = get_chunk(IDAT, fp, &offset);
    }
    if (result ==0){
        result = get_chunk(IEND, fp, &offset);
    }
    png->p_IHDR = IHDR;
    png->p_IDAT = IDAT;
    png->p_IEND = IEND;

    return result;
}

