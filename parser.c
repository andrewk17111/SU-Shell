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
#include "error.h"


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
 * Returns a string that starts at the given index at the length given
 * 
 * @param str - The source string you want to get a part of
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * 
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
 * Splits the command line input into subcommands each divided by a pipe.
 * 
 * @param cmdline: the command that was entered by user
 * @param subcommands_arr: array to hold subcommand strings
 * @param cmd_len: length of command
 */ 
void split_cmdline(char *subcommands_arr[], char *cmdline) {
    int idx = 0;   // index of subcommand array 
    int start = 0, len = 0; // subcommand start index and length

    int cmd_len = strlen(cmdline);
    for (int i=0; i<cmd_len; i++) {
        // reached pipe or the end of the cmdline input
        if (cmdline[i] == '|' || cmdline[i] == '\n') {
            // copy subcommand to array
            char *subcommand = sub_string(cmdline, start, len);
            subcommands_arr[idx++] = strdup(subcommand);
            free(subcommand); 

            // next subcommand begins at next position (after the pipe) 
            start = i+1;
            len = 0;
        } else {
            len++; // still in subcommand, increment subcommand length
        }
    }
}


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
 * Creates a new token node and adds the node to the tail of the given list
 * 
 * @param sm: state_machine struct
 * @param head: head of the linked list of tokens to add the node to
 * @param text: holds a substring that is part of a command
 **/ 
void add_token_node(struct state_machine_t *sm, struct list_head *head, char *text) {
    struct token_t *token = malloc(sizeof(struct token_t));
    token->token_text = strdup(text);
    token->token_type = TOKEN_NORMAL;

    list_add_tail(&token->list, head);
    free(text);
}


/**
 * Removes token node from linked list and frees token memory
 * 
 * @param node: node to remove from list
 * @param token: token of the node to free
 */ 
void remove_token_node(struct list_head *node, struct token_t *token) {
    list_del(node);
    free(token->token_text);
    free(token);
}


/**
 * Frees array of subcommand strings after parsing is complete
 * 
 * @param subcommands_arr: array of subcommand strings to free
 * @param num_commands: number of subcommand strings present
 */ 
void free_subcommands(char *subcommands_arr[], int num_commands) {

    // free each subcommand string
    for (int i=0; i<num_commands; i++) {
        free(subcommands_arr[i]);
    }

    // free subcommand array
    free(subcommands_arr);
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
 * Sets commands redirection out fields. First checks if this redirection was already specified and if so we
 * have a malformed command, return error. Otherwise the command struct is updated with the appropriate identifiers
 * 
 * @param command: structure to hold command representation
 * @param token: token which holds the file associated with the redirection
 * @param token_type: type of redirection 
 * 
 * @return whether malformed command was found
 */ 
int set_redirection_out(struct command_t *command, struct token_t *token, enum redirect_type_e token_type) {
    if (command->file_out != 0) {
        LOG_ERROR(ERROR_INVALID_CMDLINE);
        return RETURN_ERROR;
    }

    command->outfile = strdup(token->token_text);
    command->file_out = token_type;

    return RETURN_SUCCESS;
}


/**
 * Sets commands redirection in fields. First checks if this redirection was already specified and if so we
 * have a malformed command, return error. Otherwise the command struct is updated with the appropriate identifiers
 * 
 * @param command: structure to hold command representation
 * @param token: token which holds the file associated with the redirection
 * @param token_type: type of redirection 
 * 
 * @return whether malformed command was found
 */ 
int set_redirection_in(struct command_t *command, struct token_t *token, enum redirect_type_e token_type) {
    if (command->file_in != 0) {
        LOG_ERROR(ERROR_INVALID_CMDLINE);
        return RETURN_ERROR;
    }

    command->infile = strdup(token->token_text);
    command->file_in = token_type;

    return RETURN_SUCCESS;
}

/**
 * Checks if token type is associated with redirection. Filenames that proceed a redirection symbol in the 
 * command have these types which describe how to interact with the file.
 * 
 * @param token_type: type of token
 * 
 * @return integer true or false
 */ 
int is_redirection_token(enum token_types_e token_type) {
    return (token_type == TOKEN_FNAME_OUT_OVERWRITE || token_type == TOKEN_FNAME_OUT_APPEND || token_type == TOKEN_FNAME_IN);
}

/**
 * Iterates over all tokens to determine if redirection is present. If redirection is
 * found, updates the command structure accordingly to describe what kind of redirection
 * to handle and the corresponding filename that was specified for the redirection.
 * 
 * @param command: structure to hold command representation
 * @param head: head of the linked list of tokens
 * 
 * @return whether malformed command was found
 */ 
int set_command_redirections(struct command_t *command, struct list_head *head) {
    int rc = 0;

    struct list_head *curr;
    struct token_t *token;
    for (curr = head->next; curr != head; curr = curr->next) {
        token = list_entry(curr, struct token_t, list);
        enum token_types_e token_type = token->token_type;

        if (is_redirection_token(token_type)) {

            // >
            if (token_type == TOKEN_FNAME_OUT_OVERWRITE) {
                rc = set_redirection_out(command, token, FILE_OUT_OVERWRITE);
            }

            // >>
            if (token_type == TOKEN_FNAME_OUT_APPEND) {
                rc = set_redirection_out(command, token, FILE_OUT_APPEND);
            }
            
            // <
            if (token_type == TOKEN_FNAME_IN) {
                rc = set_redirection_in(command, token, FILE_IN);
            }

            // if any errors, return error
            if (rc < 0) return RETURN_ERROR;

            // move loop to next node and delete the filename node
            struct list_head *next = curr->next;
            remove_token_node(curr, token);
            curr = next;
        }
        
    }

    return RETURN_SUCCESS; 
}


/**
 * Takes a list of tokens and converts tokens to an array which is stored in the command
 * struct.
 * 
 * @param command: structure to hold command representation
 * @param head: linked list hold tokens of the command
 */ 
void set_command_tokens(struct command_t *command, struct list_head *head) {

    // convert list of tokens to array of tokens
    int size = list_size(head) + 1;
    char *tokens_arr[size];
    token_list_to_arr(head, tokens_arr);

    // creates space in struct to hold array of tokens and copies the array to struct
    command->tokens = malloc(size * sizeof(char *));
    for (int i = 0; i < size; ++i) {
        char *token = tokens_arr[i];
        command->tokens[i] = token;
    }

    // command name is first token in array
    command->cmd_name = command->tokens[0];

    command->num_tokens = size;
}


/**
 * Takes a list of tokens which describe a single command and creates a data structure
 * to represent all the information needed about the command.
 * 
 * @param command: structure to hold command representation
 * @param head: linked list hold tokens of the command
 * @param command_position: position of the command in the command line input
 * @param num_commands: number of commands present in the command line input
 * 
 * @return whether the conversion to command structure of successful or not
 */ 
int tokens_to_command(struct command_t *command, struct list_head *head, int command_position, int num_commands) {
    int rc = 0; 

    // Initialize no file input/output
    command->file_in = REDIRECT_NONE;
    command->file_out = REDIRECT_NONE;

    // set command pipe values
    int pipe_in = (command_position != 0) ? TRUE : FALSE;                  // if command is not first, pipe in
    int pipe_out = (command_position != num_commands - 1) ? TRUE : FALSE;  // if command is not last, pipe out
    command->pipe_in = pipe_in;
    command->pipe_out = pipe_out;

    // set command file params and check return code if command was malformed
    rc = set_command_redirections(command, head);
    if (rc < 0) return RETURN_ERROR;

    // set command tokens array
    set_command_tokens(command, head);

    return RETURN_SUCCESS;
}


/**
 * Once the list of tokens is created, this function checks if any redirection is present. If found,
 * the function determines if a filename was provided and what kind of operation should be done on
 * the file (overwrite, append, read). The operation type is stored in the corresponding file token
 * and the redirection token is deleted from the list since it is no longer needed.
 * 
 * @param head: linked list hold tokens of the command
 * 
 * @return status which descibes if all redirection was valid
 */ 
int has_valid_redirection(struct list_head *head) {
    struct list_head *curr;
    struct token_t *token;

    //Loop through all of the tokens to remove redirection tokens
    for (curr = head->next; curr != head; curr = curr->next) {
        token = list_entry(curr, struct token_t, list);
        char *tokstr = token->token_text;

        // if token text is redirection symbol
        if (strcmp(tokstr, ">") == 0 || strcmp(tokstr, ">>") == 0 || strcmp(tokstr, "<") == 0) {
            // redirection should not be last node is list
            if (curr->next == head) {
                LOG_ERROR(ERROR_INVALID_CMDLINE);
                return RETURN_ERROR;
            } else {
                // get token after redirection
                struct token_t *fname_tok = list_entry(curr->next, struct token_t, list);

                // overwrite to file
                if (strcmp(tokstr, ">") == 0)
                    fname_tok->token_type = TOKEN_FNAME_OUT_OVERWRITE;

                // append to file
                else if (strcmp(tokstr, ">>") == 0)
                    fname_tok->token_type = TOKEN_FNAME_OUT_APPEND;

                // read from file
                else if (strcmp(tokstr, "<") == 0)
                    fname_tok->token_type = TOKEN_FNAME_IN;

                // move loop to next node and delete the redirection node
                struct list_head *next = curr->next;
                remove_token_node(curr, token);
                curr = next;
            }
        }
    }

    return RETURN_SUCCESS;
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
void tokenizer(struct list_head *list_tokens, char *cmdline) {
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

    // free statemachine memory
    free(sm);
}


/**
 * Driver function for the command parser functionality. Takes a single commmand line
 * input, breaks it into an array of subcommands and parses each. Each subcommand is tokenized
 * and converted a command structure and added to the array of commands. 
 * 
 * When parser finishes, a complete array of commands is populated and ready to be executed by the shell.
 * 
 * @param commands_arr: array to hold command stucts
 * @param num_commands: number of subcommands to parse
 * @param cmdline: the command line given by the user that will be parsed
 * 
 * @return status of command parsing
 **/ 
int parse_command(struct command_t *commands_arr[], int num_commands, char *cmdline) {

    int rc;

    char **subcommands_arr = malloc(sizeof(char *) * num_commands); 
    split_cmdline(subcommands_arr, cmdline);

    // parse each subcommands
    for (int i = 0; i < num_commands; i++) {
        // Initialize token list for sub command
        LIST_HEAD(list_tokens);

        // parse tokens from subcommand and store in list
        tokenizer(&list_tokens, subcommands_arr[i]);

        // verifies token arrangement is valid for any present redirection
        rc = has_valid_redirection(&list_tokens);
        if (rc < 0) return RETURN_ERROR;

        // translate token list to subcommand structure
        struct command_t *command = malloc(sizeof(struct command_t));
        rc = tokens_to_command(command, &list_tokens, i, num_commands);
        if (rc < 0) return RETURN_ERROR;

        commands_arr[i] = command;

        // free token list for parsed subcommand
        clear_list(&list_tokens);
    }

    // free array of subcommand strings
    free_subcommands(subcommands_arr, num_commands);

    return RETURN_SUCCESS;
}
