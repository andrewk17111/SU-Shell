/**
 * @file: parser.c
 * @author: Andrew Kress
 * @author: Michael Permyashkin
 * 
 * @brief: Parses command line input 
 * 
 * Functions used to tokenize a command line string
 * into parts (which we call tokens). The parser utilizes a state machine
 * by looking at each character in turn, determining what that character is
 * and how to proceed.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "list.h"
#include "internal.h"

// Enum to hold all possible states the state machine can be in
enum state_e {
    WHITESPACE,
    CHAR,
    QUOTE
};


/**
 * The command parser uses the state pattern to determine the next
 * action based on the current character that is being read. We use
 * this structure to group all relevant information about the state 
 * machine in order to easily pass the information between the various
 * handler functions for each state.
 * 
 * @state: current state the machine is in (one of @enum state_e)   
 * @position: current position in the command line string   
 * @cmd_len: length of the command line string  
 * @sub_start: as arguments are located, this field holds the starting position
 *      of the argument that will be used when extracting the substring  
 * @sub_len: length of the substring that has been found. This field increments 
 *      until we reached the end of that argument. This is used in tandem with 
 *      @sub_start to extract the complete substring 
 **/ 
struct state_machine {
    enum state_e state;
    int position;
    int cmd_len;

    int sub_start;
    int sub_len;
};


/**
 * Initializes statemachine to starting state. The state machine is
 * initialized to begin at the 0th position of the command input
 * with a state of WHITESPACE since no characters have been encountered.
 * 
 * Initialize substring positions and command input length
 * 
 * @param sm: state_machine struct that will be used by parser
 * @param cmdline: command input from the user
 **/ 
void initialize_machine(struct state_machine *sm, char *cmdline) {
    // no characters encountered yet
    sm->state = WHITESPACE;        
    sm->position = 0;

    // store the length of the string which will be traversed
    sm->cmd_len = strlen(cmdline);

    // no substrings encountered
    sm->sub_start = 0;
    sm->sub_len = 0;
}


/**
 * Returns a string that starts at the given index at the length given
 * 
 * @param str - The source string you want to get a part of
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * @return extracted substring
 **/
char * sub_string(char* str, int start, int length) {
    char* output = calloc(length + 1, sizeof(char));
    for (int i = start; i < (start + length) && i < strlen(str); i++) {
        output[i - start] = str[i];
    }
    return output;
}


/**
 * Creates a new node in the linked list of arguments
 * 
 * @param sm: state_machine struct
 * @param list_args: head of linked list
 * @param value: argument that was extracted
 **/ 
void create_list_node(struct state_machine *sm, struct list_head *list_args, char *value) {
    struct argument_t *arg = malloc(sizeof(struct argument_t));
    arg->value = value;
    list_add_tail(&arg->list, list_args);
}


/**
 * Handler when statemachine is in WHITESPACE state. Checks if the current character
 * is a quote or character - sets state accordingly. If it is neither a character or
 * quote, we encountered another whitespace and the handler does nothing.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 **/ 
void do_ws(struct state_machine *sm, char c) {
    // if quote character is a quote
    if (c == '"') {
        // sub string will begin at next position (we do not want to include the quotes)
        sm->sub_start = sm->position + 1;
        // in WHITESPACE state, so this is the first char in substring
        sm->sub_len = 1;

        // update state
        sm->state = QUOTE;
    } 
    // character is not space(s) 
    else if (c != ' ' && c != '\t') {
        // sub string will begin at current position
        sm->sub_start = sm->position;
        // in WHITESPACE state, so this is the first char in substring
        sm->sub_len = 1;
        
        // update state
        sm->state = CHAR;
    }
}


/**
 * Handler when statemachine is in CHAR state. Checks if the current character
 * is blank space(s) - sets state accordingly. If char is neither a space(s) or
 * newline, we encountered another character so we increment the substring length.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 * @param list_args: head of linked list
 * @param cmdline: command input from user
 **/ 
void do_char(struct state_machine *sm, char c, struct list_head *list_args, char *cmdline) {
    // is space(s) or newline, we found the end of substring
    if (c == ' ' || c == '\t' || c == '\0' || c == '\n') {
        // get substring and add argument to list
        char *value = sub_string(cmdline, sm->sub_start, sm->sub_len);
        create_list_node(sm, list_args, value);

        // update state
        sm->state = WHITESPACE;
    } 
    // still inside a substring, increment substring length
    else {
        sm->sub_len++;
    }
}


/**
 * Handler when statemachine is in QUOTE state. Checks if statemachine reached end
 * of substring or command input. If char is neither a quote or newline, we found 
 * the end of a quoted substring.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 * @param list_args: linked list of arguments
 * @param cmdline: command input from user
 **/ 
void do_quote(struct state_machine *sm, char c, struct list_head *list_args, char *cmdline) {
    // if not quote or end of input, increment substring length
    if (c != '"' && c != '\0' && c != '\n') {
        sm->sub_len++;
    } 
    // we reached the end of quoted substring
    else {
        // get substring and add argument to list
        char *value = sub_string(cmdline, sm->sub_start, --sm->sub_len);
        create_list_node(sm, list_args, value);
        
        // update state
        sm->state = WHITESPACE;
    }
}


/**
 * Uses the statemachine pattern to iteratively move through the command line that
 * that given by the user. Based on the character value, the state of the machine is 
 * changed to determine what should be done next. The state machine keeps track of 
 * the length of the cmdline input, its position in the cmdline, the machines state,
 * and positions of each substring that is encountered.
 * 
 * @param list_args: linked list to hold parsed arguments from input
 * @param cmdline: the command that was entered by user
**/
void split_command(struct list_head *list_args, char *cmdline) {
    // initialize statemachine
    struct state_machine *sm = malloc(sizeof(struct state_machine));
    initialize_machine(sm, cmdline);

    // initialize statemachine
    for (sm->position = 0; sm->position < sm->cmd_len; sm->position++) {
        
        // get character the statemachine is looking at
        char c = cmdline[sm->position];

        switch(sm->state) {
            // character state
            case CHAR:
                do_char(sm, c, list_args, cmdline);
                break;

            // quote state
            case QUOTE:
                do_quote(sm, c, list_args, cmdline);
                break;

            // whitespace state
            default:
                do_ws(sm, c);

        }
    }
}


/**
 * Driver function for the command parser functionality. This function initializes
 * an empty linked list that will hold the parsed arguments. Then it simply 
 * passes the linked list and the command line to the parsing function.
 * 
 * @param cmdline: the command line given by the user that will be parsed
 **/ 
void handle_command(char *cmdline) {
    // initilize linked list to hold tokens
    LIST_HEAD(list_args);

    // parse command
    split_command(&list_args, cmdline);

    list_print(&list_args);
    printf("size -> %d\n", list_size(&list_args));
}