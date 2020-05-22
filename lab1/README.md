Introduction:
    This is the Lab 1 code for ECE 252 written by Devon Miller-Junk and Braden Oldejans Bakker.

    The files in the starter directory in this repo are forked from Bernie Roehl, the lab instructor for this course and not written by Devon or Braden.

    The code is written for the purpose of ECE 252 Lab1 in Summer 2020.

Starter Code Explanation:

    cmd_arg -> demonstrates how to capture command line input arguments.

    ls -> how to list all files under a directory (and obtain file types).

    png_util -> functions to process a PNG image file

    pointer -> demonstrates how to use pointers to access a C structure

    images -> sample image files

    segfault -> contains a broken program that has a segmentation fault bug, used for practicing gdb and ddd.

Purpose and Goals:

    -Using linux commands in C.
    -Navigating the file system in C.

Deliverables

    1. Place everything under the Lab1 directory in ECE252 directory in eceubuntu. Put all source code here.

    2. Create a Makefile that will target pnginfo, catpng, and findpng. Also add support for make clean to remove those executables and all .o files.

    3. Use the zip command to zip up the contents, name it lab1.zip. They will run unzip lab1.zip to unzip it and produce a Lab1 sub-directiory in the current working directory. Source code and Makefile will be inside Lab1.

    4. Submite lab1.zip to learn.

Create:

    Required:
        - pnginfo: 

            Purpose: prints the dimensions of a valid PNG file or prints an error message.
                If the input file is a PNG, but contains CRC errors, use the same output as the pngcheck command.

            Parameter: the pathname of the file (i.e. WEEF_1.png)

            example:
                1. Default
                    input:
                        ./pnginfo WEEF_1.png
                    output:
                        WEEF_1.png: 450 x 229

                2. Not a PNG file
                    input:
                        ./pnginfo Disguise.png
                    output:
                        Disguise.png: Not a PNG file 

                3. CRC Errors
                    input:
                        ./pnginfo red-green-16x16-corrupted.png
                    output:
                        red-green-16x16-corrupted.png: 16 x 16
                        IDAT chunk CRC error: computed 34324f1e, expected dc5f7b84

        - findpng:

            Purpose: search a given directory hierarchy to find all PNG files under it. 
            Output is to list all of the relative pathnames of the PNG files (one per line).
            No ordering required.
            If the search result is empty, output: "findpng: No PNG file found".

            Parameter: root of the directory tree.

            example:
                1. Default
                    input:
                        findpng .
                    output:
                        ./lab1/sandbox/new_bak.png
                        ./lab1/sandbox/t1.png
                        ./png_img/rgba_scanline.png
                        ./png_img/v1.png

                2. No PNG found
                    input:
                        findpng .
                    output:
                        findpng: No PNG file found

            notes: see the navigating a directory tree section of this readme file.

        - catpng:

            Purpose: Given a directory of PNG files, (each PNG file is a horizontal strip of the same width, but differing heights, that form a larger image) concatenate all the PNG images into one PNG image. The files are named according to the convention *_N.png where N is
            a series of consecutive increasing numbers (0, 1, 2, 3, 4, ...), which indicate the position of the image from top to bottom.
            The resulting combined PNG should be called all.png.

            Parameter: array of PNG files (relative paths to the png files)

            example:
                1. Default
                    input:
                        catpng ./img1.png ./png/img2.png

    Recommendations:
        - is_png() function that takes eight bytes and checks if it is a png file signature.

        - get_data_IHDR() function that extracts the image meta information (including height and width) from the IHDR chunk.

Navigating a Directory Tree

    3 types of files (that we care about in this section):

        Regular files: like PNGs, Text documents, etc.

        Directories: a directory is a directory file.

        symbolic Links: a link created by ln -s.

        - Use the file ls/ls_ftype/c to determine the type of a given file (see the starter code).
            Note: the struct dirent returned by the readdir() function has a field d_type that also gives the file type info. (not supported by all file systems).
            -> Need to test this before submission.

    Navigating to a leaf node:

        opendir: returns a directory stream for readdir to read each entry in a directory. Call closedir after the operation is complete.
            - Use to check each file type in a directory.
            - If a regular file, check if it is a PNG
            - If it is a directory, recursively navigate it
            - Check ls/ls_fname.c in the starter code to see how to list all the file entries in a given directory

File I/O

    System Level (unbuffered) functions: open, read, write, lseek, close. For the most part we will not use these.

    Library Level: built on top of unbuffered I/O functions, and handles buffer allocation and minimizing the number of read&write calls.

        fopen: returns a file pointer given a file name and mode. Use modes rb for reading and wb+ for writing since a PNG is a binary file.

        fread: after opening the file, read some number of bytes from the stream pointed to by the FILE pointer opened by fopen.

        fwrite: write the user-specified number of bytes to the stream pointed to by the FILE pointer.

        fseek: set the file position indicator.

        fclose: close after finished.

        ftell: returns file position of a given stream.

PNG File Format

    -Each pixel is represented by 4 8-bit unsigned integers (0-255 for RGBA)

    -Uses Big-Endian byte order (the same as the network byte order)

        -Use ntohl and htonl to convert between network order and host oder. 

    Format:

        1. 8-Byte Header

            a. 89

            b. 3 Bytes spelling out PNG in ASCII (50 4E 47)

            c. 0D 0A 1A 0A


        2. IHDR Chunk

            a. 4 Byte Length Field

                The length of the data in bytes.
                The length of the IHDR is 13. 

            b. 4 Byte Type Field

                IHDR in ASCII

            c. Data (Length number of bytes)

                i. 4 Byte Width (unsigned int)

                ii. 4 Byte Height (unsigned int)

                iii. 1 Byte Bit Depth (value of 8 for this assignment)

                iv. 1 Byte Colour Type (value of 6 for this assignment since RGBA)

                v. 1 Byte Compression Method (value of 0)

                vi. 1 Byte Filter Method (value of 0)

                vii. 1 Byte Interlace Method (value of 0 in this assignment, can also be 1)

            d. 4 Byte CRC Field

                Cyclic redundancy check on the Type and Data fields.
                -Note: use the png_util crc function to calculate the CRC value

        3. IDAT Chunk

            a. 4 Byte Length Field

                The length of the data in bytes.

            b. 4 Byte Type Field

                IDAT in ASCII

            c. Data (Length number of bytes)

                Contains the compressed, filtered pixel data.
                The first byte in each scanline indicates the filter method used.

            d. 4 Byte CRC Field

                Cyclic redundancy check on the Type and Data fields.
                -Note: use the png_util crc function to calculate the CRC value

        4. IEND Chunk

            a. 4 Byte Length Field

                The length of the data in bytes.

            b. 4 Byte Type Field

                IEND in ASCII

            c. Data (Length number of bytes)

                Empty since IEND just marks the end of the PNG file.

            d. 4 Byte CRC Field

                Cyclic redundancy check on the Type and Data fields.
                -Note: use the png_util crc function to calculate the CRC value

Concatenate the Pixel Data

    This is not the approach you would think but it works.

    Steps:

        1. Start with filtered pixel data of each image. 
        
        2. Concatenate the 2 chunks of filtered pixel data arrays vertically
        
        3. Apply the compression method to generate the data field of the new IDAT chunk.

        4. Update the IHDR chunk with the new height. 

    Why?

        Since the IDAT chunk is compressed using zlib format 1.0, we can use inflate (uncompress) on the data. The mem_inf function in the starter code takes in-memory deflated (compressed) data as input and stores the uncompressed data in a given memory location. For each IDAT chunk, call this function and stack the returned data in the order you wish to obtain the concatenated filtered pixel array. Then turn it into an IDAT chunk, compress it again using mem_def in the starter code.

More Function Examples

    opendir: opens a directory stream named by the dirname argument.

    closedir: close the directory stream

    readdir: returns a pointer to a structure represent the directory entry at the current position in the directory stream.
            Then repositions the directory stream at the next entry.

    stat:

    fstat:

