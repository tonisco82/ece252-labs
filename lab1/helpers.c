/**
 * @brief: Helper Functions for ECE 252 Lab Course
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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
int get_data_IHDR(char* IHDR_chunk, unsigned int* width, unsigned int* height){

    *width = 0;
    *height = 0;

    // This is assuming little endian data formatting, 
    // reverse the order of the i for big-endian
    for(int i=3;i>-1;i--){
        *height = ((*height) << 8);
        *height += (unsigned int)IHDR_chunk[i + 12];

        *width = ((*width) << 8);
        *width += (unsigned int)IHDR_chunk[i + 8];

        printf("Current Width: %d\n", *width);
        printf("Current Height: %d\n", *height);
    }

    return 0;
}

int main(int argc, char *argv[]) {

    char* test = malloc(25*sizeof(char));

    for(int i=0;i<25;i++){
        test[i] = 'a';
    }
    test[24] = '\0';

    printf("Test: %s\n", test);

    unsigned int* input_width = (unsigned int*)test + 8;
    //unsigned int* input_height = (unsigned int*)test + 12;

    *input_width = 12191;

    printf("Test: %s\n", test);

    unsigned int width = 0;
    unsigned int height = 0;

    printf("%d\n", get_data_IHDR(test, &width, &height));

    printf("Width: %d\n", width);
    printf("Height: %d\n", height);

    free(test);
}


