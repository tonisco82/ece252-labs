#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils/file_utils/file_fns.c"
#include "utils/png_utils/png_fns.c"

/**
 * @brief: Shows examples of how to use the utility functions
 */
int main(int argc, char *argv[]){
    char *weef_img = "WEEF_1.png";
    char *all_img = "hopefullythisworks.png";

    void *data1;
    void *data2;
    void *combinedData;
    unsigned long length1;
    unsigned long length2;
    unsigned long combinedLength;

    simple_PNG_p png1 = (simple_PNG_p)(malloc(sizeof(struct simple_PNG)));
    simple_PNG_p png2 = (simple_PNG_p)(malloc(sizeof(struct simple_PNG)));
    simple_PNG_p combined = (simple_PNG_p)(malloc(sizeof(struct simple_PNG)));

    printf("1.write_file_to_mem %d\n", write_file_to_mem(&data1, &length1, weef_img));
    printf("2.write_file_to_mem %d\n", write_file_to_mem(&data2, &length2, all_img));

    printf("1.fill_png_struct %d\n", fill_png_struct(png1, data1));
    printf("2.fill_png_struct %d\n", fill_png_struct(png2, data2));

    printf("Combine PNGs %d\n", combine_pngs(combined, png1, png2));

    printf("Writing combined PNG to data %d\n", fill_png_data(&combinedData, &combinedLength, combined));

    printf("Writing data to a file %d\n", write_mem_to_file(combinedData, combinedLength, "hopefullythisworks2.png"));

    free(data1);
    free(data2);
    free(combinedData);
    free_simple_PNG(png1);
    free_simple_PNG(png2);
    free_simple_PNG(combined);
    return 0;
}