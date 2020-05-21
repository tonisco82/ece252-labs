/**
 * @brief: Prints the dimensions of a valid PNG file or prints an error message. 
 *          If the input file is a PNG, but contains CRC errors, use the same 
 *          output as the pngcheck command.
 * EXAMPLE: ./pnginfo WEEF_1.png
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Usage example: ./pnginfo WEEF_1.png\n");
        return -1;
    }



    return 0;
}