/**
 * @file: internal.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file to define shared functions and structures across the shell
 */ 


#include <stddef.h>
#include "list.h"

#ifndef CMDLINE_H
#define CMDLINE_H

/**
 * Definition of all types that any given token can be
 * 
 * TOKEN_NORMAL: token is command, word, arguments, flags
 * TOKEN_REDIR: token is a redirection (>, >>, <)
 * TOKEN_FNAME: token is a file name
 */
enum token_types_e {
    TOKEN_NORMAL,
    TOKEN_REDIR,
    TOKEN_FNAME
};


/**
 * The command parser extracts parts of a given command which we call tokens. 
 * These parts are stored in this structure to hold all relevant information
 * needed throughout the shell about each token.
 * 
 * @token_text: the literal string value of the token
 * @token_type: described by a value of the token_types_e enum, this is used
 *      to identify the role of the token within the command
 * @list: each token is a member of a list 
 **/ 
struct token_t {
    char *token_text;
    enum token_types_e token_type;
    struct list_head list;
};


struct command_t {
    struct list_head tokens;
    struct list_head list;
};


/**
 * Driver function for the command parser functionality. This function initializes
 * an empty linked list that will hold the parsed arguments. Then it simply 
 * passes the linked list and the command line to the parsing function.
 * 
 * @param cmdline: the command line given by the user that will be parsed
 * @param len: length of cmdline string
 * 
 * @return: status of execution
 **/ 
int handle_command(char* cmdline, int len);

#endif