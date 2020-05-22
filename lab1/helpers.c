/**
 * @brief: Helper Functions for ECE 252 Lab Course
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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

// Place the IHDR information into the destination location.
// Destination should have 13 bytes of space available.
// For information about how the IHDR data is formatted see the Readme
int get_data_IHDR(char* file_path, char*destination);



