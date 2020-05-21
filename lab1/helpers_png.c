/**
 * @brief: Helper Functions for PNGs
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

bool is_png(U8 *source,  U64 source_len);

get_data_IHDR(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len);

