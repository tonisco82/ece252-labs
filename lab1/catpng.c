/**
 * @brief: Given a directory of PNG files, (each PNG file is a horizontal strip of 
 *         the same width, but differing heights, that form a larger image) concatenate 
 *         all the PNG images into one PNG image. The files are named according to the 
 *         convention *_N.png where N is a series of consecutive increasing numbers 
 *         (0, 1, 2, 3, 4, ...), which indicate the position of the image from top to bottom.
 *         The resulting combined PNG should be called all.png.
 * EXAMPLE: ./catpng ./img1.png ./png/img2.png
 */