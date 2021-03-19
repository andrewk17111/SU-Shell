/**
 * @file: internal.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file to define shared functions and structures across the shell
 */ 


#include <stddef.h>
#include "list.h"

#ifndef INTERNAL_H
#define INTERNAL_H

struct argument_t {
    char *value;
    struct list_head list;
};


/**
 * Driver function for the command parser functionality. This function initializes
 * an empty linked list that will hold the parsed arguments. Then it simply 
 * passes the linked list and the command line to the parsing function.
 * 
 * @param cmdline: the command line given by the user that will be parsed
 * @param len: length of cmdline string
 **/ 
void handle_command(char* cmdline, int len);

#endif