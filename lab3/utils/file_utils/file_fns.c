/**
 * @brief: File related utility functions for ECE 252
 * Written by Devon Miller-Junk and Braden Bakker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#pragma once


// Returns the size of the file in number of bytes
// -1 on error
// file_path is the file path to the file
// If the file is a symbolic link, it returns the length of the pathname to the link
int get_file_size(char* file_path){

    // Stats Struct
    struct stat stats;

    // Check File type
    if(stat(relative_path, &stats) == 0) return stats.st_size;
    return -1;
}

// Returns the type of the file
// file_path is the file path to the file
// Return values:
// -1: Error in reading file
// 0: regular file
// 1: Directory
// 2: Block Special File
// 3: Character Special File
// 4: Pipe
// 5: Symbolic Link
// 6: Socket
// 7: Unkown File Type
int get_file_type(char* file_path){

    // Stats Struct
    struct stat stats;

    // Check File type
    if(stat(relative_path, &stats) == 0) {
        if(S_ISDIR(stats.st_mode)) {
            //Is a directory
            return 1;
        }else if(S_ISREG(stats.st_mode)) {
            //Is a regular file
            return 0;
        }else if(S_ISBLK(stats.st_mode)) {
            //Is a block special file
            return 2;
        }else if(S_ISCHR(stats.st_mode)) {
            //Is a character special file
            return 3;
        }else if(S_ISFIFO(stats.st_mode)) {
            //Is a pipe
            return 4;
        }else if(S_ISLNK(stats.st_mode)) {
            //Is a symbolic link
            return 5;
        }else if(S_ISSOCK(stats.st_mode)) {
            //Is a socket
            return 6;
        }else{
            return 7;
        }
    }
    return -1;
}

// Takes a regular file path and writes the file to memory
// Changes the result pointer to point to the memory
// Changes the value of length to the length of the memory in bytes
// Return value:
// -1: Error in reading file, memory has been cleaned up
// 0: success, *result is a pointer to memory that needs to be cleaned up
int write_file_to_mem(void** result, int* length, char *file_path){
    if(get_file_type(file_path) !== 0) return -1; //Make sure regular file

    int file_len = get_file_size(file_path); //Get file size
    if(file_len == -1) return -1;
    
    FILE *fp = fopen(file_path, "rb"); //Open file

    if(!fp) return -1;

    *result = malloc(file_len); //Allocate data
    *length = file_len;

    int fread_status = fread(*result, file_len, 1, fp); //Read file

    fclose(fp);

    if(fread_status != 1){
        free(*result);
        return -1;
    }

    return 0;
}

// Takes data from memory and writes it to a file
// Return value:
// -1: Error in writing to file
// 0: success
int write_mem_to_file(void* result, int length, char *file_path){
    FILE *fp = fopen (file_path, "wb+");

    if(!fp) return -1;

    fwrite(result, length, 1, fp);

    fclose(fp);

    return 0;
}
