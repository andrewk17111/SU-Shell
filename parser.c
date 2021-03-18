/**
 * File: parser.c
 * Purpose: Functions used to tokenize a command line input string
 *      into parts (which we call tokens). The parser utilizes a state machine
 *      by looking at each character in turn, determining what that character is
 *      and how to proceed.
 * 
 * @author: Andrew Kress
 **/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "internal.h"

//The current state of the state machine
enum state {
    WHITESPACE,
    CHAR,
    QUOTE
};

/**
 * Returns a string that starts at the given index at the length given
 * 
 * @param str - The source string you want to get a part of
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * @return - The specified substring
 **/
char* sub_string(char* str, int start, int length) {
    char* output = calloc(length + 1, sizeof(char));
    for (int i = start; i < (start + length) && i < strlen(str); i++) {
        output[i - start] = str[i];
    }
    return output;
}

/**
 * Separates a string by spaces or quotes
 * 
 * @param cmdline - the command that was entered by user
**/
void split_command(struct list_head *list_args, char* cmdline) {
    int cmdline_len = strlen(cmdline);
    int count = 0;
    enum state current_state = WHITESPACE;
    int start = 0;
    int length = 1;
    
    for (int i = 0; i <= cmdline_len; i++) {
        if (current_state == WHITESPACE) {
            if (cmdline[i] == '"' && i < cmdline_len - 1) {
                current_state = QUOTE;
                start = i + 1;
                length = 1;
            } else if (cmdline[i] != ' ' && cmdline[i] != '\t') {
                current_state = CHAR;
                start = i;
                length = 1;
            }
        } else if (current_state == CHAR) {
            if (cmdline[i] == ' ' || cmdline[i] == '\t' || cmdline[i] == '\0' || cmdline[i] == '\n') {
                current_state = WHITESPACE;
                count++;

                struct argument_t *arg = malloc(sizeof(struct argument_t));
                arg->value = sub_string(cmdline, start, length);
                list_add_tail(&arg->list, list_args);
            }
            length++;
        } else if (current_state == QUOTE) {
            if (cmdline[i] != '"' && cmdline[i] != '\0' && cmdline[i] != '\n') {
                length++;
            } else {
                current_state = WHITESPACE;
                count++;
                
                struct argument_t *arg = malloc(sizeof(struct argument_t));
                arg->value = sub_string(cmdline, start, --length);
                list_add_tail(&arg->list, list_args);
            }
        }
    }
}

void handle_command(char *cmdline) {
    // initilize linked list to hold tokens
    LIST_HEAD(list_args);

    split_command(&list_args, cmdline);

    list_print(&list_args);
}