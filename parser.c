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
#include "cmdline.h"

/**
 * Definition of all possible states the state machine can be in
 * 
 * WHITESPACE: token is command, word, arguments, flags
 * CHAR: token is a redirection (>, >>, <)
 * QUOTE: token is a file name
 */
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
struct state_machine_t {
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
void initialize_machine(struct state_machine_t *sm, char *cmdline) {
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
 * Creates a new token node and adds the node to the tail of the given list
 * 
 * @param sm: state_machine struct
 * @param head: head of linked list we are adding the node to
 * @param text: holds a substring that is part of a command
 **/ 
void add_token_node(struct state_machine_t *sm, struct list_head *head, char *text) {
    struct token_t *token = malloc(sizeof(struct token_t));
    token->token_text = strdup(text);
    token->token_type = TOKEN_NORMAL;

    list_add_tail(&token->list, head);
}


/**
 * Handler when statemachine is in WHITESPACE state. Checks if the current character
 * is a quote or character - sets state accordingly. If it is neither a character or
 * quote, we encountered another whitespace and the handler does nothing.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 **/ 
void do_ws(struct state_machine_t *sm, char c) {
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
    // otherwise more whitespace was encountered, do nothing
}


/**
 * Handler when statemachine is in CHAR state. Checks if the current character
 * is blank space(s) - sets state accordingly. If char is neither a space(s) or
 * newline, we encountered another character so we increment the substring length.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 * @param list_tokens: linked list to hold the subcommand being parsed
 * @param cmdline: command input from user
 **/ 
void do_char(struct state_machine_t *sm, char c, struct list_head *list_tokens, char *cmdline) {
    // is space(s) or newline, we found the end of substring
    if (c == ' ' || c == '\t' || c == '\0') {
        // get substring and add argument to list
        char *text = sub_string(cmdline, sm->sub_start, sm->sub_len);
        add_token_node(sm, list_tokens, text);

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
 * @param list_tokens: linked list to hold the subcommand being parsed
 * @param cmdline: command input from user
 **/ 
void do_quote(struct state_machine_t *sm, char c, struct list_head *list_tokens, char *cmdline) {
    // if not quote or end of input, increment substring length
    if (c != '"' && c != '\0') {
        sm->sub_len++;
    } 
    // we reached the end of quoted substring
    else {
        // get substring and add argument to list
        char *text = sub_string(cmdline, sm->sub_start, --sm->sub_len);
        add_token_node(sm, list_tokens, text);
        
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
 * @param list_tokens: linked list to hold the subcommand being parsed
 * @param cmdline: the command that was entered by user
**/
void subcommand_parser(struct list_head *list_tokens, char *cmdline) {
    // initialize statemachine
    struct state_machine_t *sm = malloc(sizeof(struct state_machine_t));
    initialize_machine(sm, cmdline);

    // initialize statemachine
    for (sm->position = 0; sm->position < sm->cmd_len; sm->position++) {
        
        // get character the statemachine is looking at
        char c = cmdline[sm->position];

        switch(sm->state) {
            // character state
            case CHAR:
                do_char(sm, c, list_tokens, cmdline);
                break;

            // quote state
            case QUOTE:
                do_quote(sm, c, list_tokens, cmdline);
                break;

            // whitespace state
            default:
                do_ws(sm, c);

        }
    }

    // Once state machine reaches end, if not in WHITESPACE state then a last substring remains
    if (sm->state != WHITESPACE) {
        char *value = sub_string(cmdline, sm->sub_start, sm->position);
        add_token_node(sm, list_tokens, value);
    }

}


/**
 * Counts the number of subcommands present in a given command line. Each time
 * a pipe is encountered we incremenet the counter and return the final value.
 * 
 * @param cmdline: the command that was entered by user
 * @param cmd_len: length of command
 * @return: interger value of the number of subcommands found
 */ 
int get_num_subcommands(char *cmdline, int cmd_len) {
    int count = 1;
    for (int i=0; i<cmd_len; i++) {
        if (cmdline[i] == '|') {
            count++;
        }
    }
    return count;
}


/**
 * Splits the command line input into subcommands each divided by a pipe.
 * 
 * @param cmdline: the command that was entered by user
 * @param subcommands_arr: array to hold subcommand strings
 * @param cmd_len: length of command
 */ 
void split_cmdline(char *subcommands_arr[], char *cmdline, int cmd_len) {
    int idx = 0;   // index of subcommand array 
    int start = 0, len = 0; // subcommand start index and length

    for (int i=0; i<cmd_len; i++) {
        // reached pipe or the end of the cmdline input
        if (cmdline[i] == '|' || cmdline[i] == '\n') {
            // copy subcommand to array
            subcommands_arr[idx++] = strdup(sub_string(cmdline, start, len)); 

            // next subcommand begins at next position (after the pipe) 
            start = i+1;
            len = 0;
        } else {
            len++; // still in subcommand, increment subcommand length
        }
    }
}


void token_list_to_arr (char **tokens_arr, struct list_head *list_tokens) {
    int size = list_size(list_tokens);
    tokens_arr = malloc(sizeof(struct token_t) * (size+1));
    list_to_arr(list_tokens, tokens_arr);


    for (int i=0; i<size+1; i++) {
        printf("[%d] -> %s\n", i, tokens_arr[i]);
    }
}


/**
 * Driver function for the command parser functionality. This function initializes
 * an empty linked list that will hold the parsed arguments. Then it simply 
 * passes the linked list and the command line to the parsing function.
 * 
 * @param cmdline: the command line given by the user that will be parsed
 * @param cmd_len: length of command
 **/ 
int handle_command(char *cmdline, int cmd_len) {
    if (cmdline == NULL) return -1;
    if (cmd_len <= 0) return -1;

    // initilize linked list to hold tokens
    LIST_HEAD(list_commands);

    // split command line into subcommands
    int sub_count = get_num_subcommands(cmdline, cmd_len);

    char *subcommands_arr[sub_count]; 
    split_cmdline(subcommands_arr, cmdline, cmd_len);

    // parse subcommands
    for (int i=0; i<sub_count; i++) {
        // Initialize list to hold subcommand
        LIST_HEAD(list_tokens);
        subcommand_parser(&list_tokens, subcommands_arr[i]);

        print_subcommand(&list_tokens);

        char **tokens_arr;
        token_list_to_arr(tokens_arr, &list_tokens);
        
    }

    return 0;
}

