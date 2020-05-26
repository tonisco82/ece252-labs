/**
 * @brief: search a given directory hierarchy to find all PNG files under it. 
 *         Output is to list all of the relative pathnames of the PNG files (one per line).
 *         No ordering required.
 *         If the search result is empty, output: "findpng: No PNG file found".
 * EXAMPLE: ./findpng .
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "linkedList.c"
#include "helpers.c"

// Declare recursive find pngs function
int findPNGs(char* curr_dir, Node_t **png_list);

int main(int argc, char *argv[]) {

    // Input checking
    if(argc != 2) {
        printf("Usage example: ./findpng .\n");
        return -1;
    }

    int find_pngs_status;

    // Declare linked list
    Node_t *linked_list = NULL;

    // Check if file path last character is a / to remove it
    char * last_char = (char *)(argv[1] + strlen(argv[1]) - 1);
    if(strncmp("/", last_char, 1) == 0){
        argv[1][strlen(argv[1]) - 1] = '\0';
    }

    // Recursively navigate through the directory to find the PNGs
    find_pngs_status = findPNGs(argv[1], &linked_list);

    if(find_pngs_status != 0){
        printf("Error Occured in Finding the PNG, exited with error code %d", find_pngs_status);
    }

    // Print the list of PNG's in order.
    scanList(linked_list, printString);

    if(linked_list == NULL){
        printf("findpng: No PNG file found");
    }

    // Release memory for the linked list
    freeMemory(linked_list);

    return 0;
}


/**
 * @brief: Go through a directory, add all PNGs to the list
 *          recursively call this function on subdirectories.
 * @return_values:
        0: Success
        -1: Error: Could not open directory
        1: Error: Could not find stats for a file
 */
int findPNGs(char* curr_dir, Node_t **png_list) {

    // Return value
    int result = 0;
    // Directory Entry
    struct dirent *dir_entry;
    // Stats Struct
    struct stat stats;
    // Relative Path of Each File
    char *relative_path;

    // Pointer to the directory
    DIR *this_dir = opendir(curr_dir);

    // Failure Check
    if(this_dir == NULL) {
        printf("Could not open directory %s\n", curr_dir);
        return -1;
    }

    // Loop over files in the directory
    while ((dir_entry = readdir(this_dir)) != NULL) {
        if(strncmp(".", dir_entry->d_name, 1) != 0) {
            //Create relative file path
            relative_path = malloc((strlen(curr_dir) + strlen(dir_entry->d_name) + 2) * (sizeof(char)));
            // Make Relative Path
            strcpy(relative_path, curr_dir);
            strcat(relative_path, "/");
            strcat(relative_path, dir_entry->d_name);

            // Check File type
            if(stat(relative_path, &stats) == 0) {
                if(S_ISDIR(stats.st_mode)) {
                    //Is a directory

                    // Call Find PNGs recursively
                    result = findPNGs(relative_path, png_list);

                } else if(S_ISREG(stats.st_mode)) {
                    //Is a regular file

                    // Check if PNG
                    if(is_png(relative_path) == 1){
                        //Place into linked list
                        char *temp_data = malloc((strlen(relative_path) + 1) * (sizeof(char)));
                        strcpy(temp_data, relative_path);
                        push(png_list, temp_data);
                    }
                }

            } else {
                printf("Error: Could not open file stats %s\n", relative_path);
                result = 1;
            }

            free(relative_path);
        }
    }
    // Close directory
    closedir(this_dir);
    return result;
}