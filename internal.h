#include <stddef.h>
#include "list.h"

#ifndef INTERNAL_H
#define INTERNAL_H

struct argument_t {
    char *value;
    struct list_head list;
};

/**
 * Returns a string that starts at the given index at the length given
 * 
 * @param str - The source string you want to get a part of
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * @return - The specified substring
 **/
char* sub_string(char* str, int start, int length);

/**
 * Separates a string by spaces or quotes
 * 
 * @param str - The source string you want split
**/
void handle_command(char* str);


#endif