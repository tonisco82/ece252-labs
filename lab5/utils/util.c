#include <stdio.h>
#include <stdlib.h>

/**
 * @brief: Just some general functions that could be helpful
 */

/**
 * @return: the number of digits in the integer
 */
int lenOfNumber(int num){
    int result = 1;
    int temp = num < 0 ? num * -1 : num;
    while(temp >= 10){
        result++;
        temp = temp/10;
    }
    return result;
}