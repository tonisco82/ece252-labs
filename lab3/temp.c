#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils/file_utils/file_fns.c"
#include "utils/png_utils/png_fns.c"


int main(int argc, char *argv[]){
    char *weef_img = "WEEF_1.png";
    void *data;
    unsigned long length = 0;

    simple_PNG_p png_data = malloc(sizeof(struct simple_PNG));

    void *resultData;
    unsigned long result_size;

    printf("The size of the file is %d\n", get_file_size(weef_img));
    printf("The type of the file is %d\n", get_file_type(weef_img));
    printf("Result from write file to memory %d\n", write_file_to_mem(&data, &length, weef_img));
    printf("The length of the data is %ld\n", length);

    printf("Result of filling the png %d\n", fill_png_struct(png_data, data));

    printf("Result of filling the data from the png %d\n", fill_png_data(&resultData, &result_size, png_data));
    printf("The length of the result data is %ld\n", result_size);

    printf("Result from write file to memory %d\n", write_mem_to_file(resultData, result_size, "all.png"));
    free(data);
    free(resultData);
    free_simple_PNG(png_data);

    return 0;
}