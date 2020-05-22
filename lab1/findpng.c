/**
 * @brief: search a given directory hierarchy to find all PNG files under it. 
 *         Output is to list all of the relative pathnames of the PNG files (one per line).
 *         No ordering required.
 *         If the search result is empty, output: "findpng: No PNG file found".
 * EXAMPLE: ./findpng .
 */



int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Usage example: ./findpng .\n");
        return -1;
    }



    return 0;
}