/**
 * @file: parser.c
 * @author: Andrew Kress
 * @author: Michael Permyashkin
 * 
 * @brief: Parses command line input 
 * 
 * Parses command line input into an array of command structs which hold all information
 * needed by the execution units. The parser uses a state machine
 * by looking at each character in turn, determining what that character is
 * and how to proceed.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "list.h"
#include "cmdline.h"
#include "error.h"


// Definition of all types that any given token can be in a command line input
enum token_types_e {
    TOKEN_NORMAL,
    TOKEN_REDIR,
    TOKEN_FNAME_IN,
    TOKEN_FNAME_OUT_OVERWRITE,
    TOKEN_FNAME_OUT_APPEND,
};


// Parser builds list of token found in command line
struct token_t {
    char *token_text;
    enum token_types_e token_type;
    struct list_head list;
};


// Definition of all possible states the state machine can be in
enum state_e {
    WHITESPACE,
    CHAR,
    QUOTE
};


// Structure to hold all information the statemachine needs as it parses tokens from command
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
 * @return extracted substring
 */
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
        if (cmdline[i] == '|' || cmdline[i] == '\n' || cmdline[i] == '\0') {
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
 */ 
void initialize_statemachine(struct state_machine_t *sm, char *cmdline) {
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
 * Frees allocated memory after parser is complete
 * 
 * @param sm: statemachine used by tokenizer function 
 * @param subcommands_arr: array of subcommand strings to free
 * @param num_commands: number of subcommand strings present
 */ 
void parser_clean_up(struct state_machine_t *sm, char **subcommands_arr, int num_commands) {
    // free each subcommand in array
    for (int i=0; i<num_commands; i++) {
        free(subcommands_arr[i]);
    }
    // free subcommand array
    free(subcommands_arr);

    // free statemachine memory
    free(sm);
}


/**
 * Converts linked list of tokens to an array of tokens
 * 
 * @param head: the head node of a given linked list
 * @param arr: array to hold token text
 */ 
void token_list_to_arr(struct list_head *head, char **arr) {
    if (list_empty(head)) return;

    int i = 0;
    struct token_t *token; 
    struct list_head *curr;

    // copy each tokens text into array
    for (curr = head->next; curr != head; curr = curr->next) {
        token = list_entry(curr, struct token_t, list);
        arr[i++] = strdup(token->token_text);
    }
    // terminate array with null
    arr[i] = NULL; 
}


/**
 * Creates a new token node and adds the node to the tail of the given list
 * 
 * @param sm: state_machine struct
 * @param head: head of the linked list of tokens to add the node to
 * @param text: holds a substring that is part of a command
 */ 
void add_token_node(struct state_machine_t *sm, struct list_head *head, char *text) {
    struct token_t *token = malloc(sizeof(struct token_t));
    token->token_text = strdup(text);
    token->token_type = TOKEN_NORMAL;

    list_add_tail(&token->list, head);
    free(text);

    // reset statemachine before it moves to next iteration
    sm->sub_start = sm->position;
    sm->sub_len = 0;
    sm->state = WHITESPACE;
}


/**
 * Removes token node from linked list and frees token memory
 * 
 * @param node: node to remove from list
 * @param token: token of the node to free
 */ 
void remove_token_node(struct list_head *node, struct token_t *token) {
    // remove node from list
    list_del(node);
    // free token text memory
    free(token->token_text);
    // free token struct
    free(token);
}


/**
 * Free all allocated memory to hold list of tokens
 * 
 * @param head: head node of list to free
 */ 
void clear_list(struct list_head *head) {
    struct token_t *token; 

    // traverse until no nodes remain in list
    while (!list_empty(head)) {
        token = list_entry(head->next, struct token_t, list);
        remove_token_node(&token->list, token);
    }
}


/**
 * Checks if token type is associated with redirection. Filenames that proceed a redirection symbol in the 
 * command have these types which describe how to interact with the file.
 * 
 * @param token_type: type of token
 * @return true or false
 */ 
int is_redirection_token(enum token_types_e token_type) {
    return (token_type == TOKEN_FNAME_OUT_OVERWRITE || token_type == TOKEN_FNAME_OUT_APPEND || token_type == TOKEN_FNAME_IN);
}


/**
 * Checks if token text is a redirection. Redirection tokens are >, >>, <
 * 
 * @param token_text: text of token
 * @return true or false
 */ 
int is_redirection_text(char *token_type) {
    return (strcmp(token_type, ">") == 0 || strcmp(token_type, ">>") == 0 || strcmp(token_type, "<") == 0);
}


/**
 * Verifies that stdin of a command is valid. Verifies that a command can either
 * read from a pipe or read from a file, not both.
 * 
 * If stdin is from to a file, the function verifies that a filename is present.
 * 
 * @param pipe_out: true/false whether command reads from pipe 
 * @param redir_type: type of file redirection commmand holds
 * @param out_fname: name of file for redirection
 * @return true or false
 */ 
int is_valid_stdin(int pipe_in, enum redirect_type_e redir_type, char *in_fname) {
    // stdin can only be redirected once
    if (pipe_in && redir_type == FILE_IN) {
        return ERROR;
    }

    // if rediction in but no filename, error
    if (redir_type == FILE_IN && in_fname == NULL) {
        return ERROR;
    }

    return SUCCESS;
}


/**
 * Verifies that stdout of a command is valid. Verifies that a command can either
 * write to a pipe or write to a file, not both.
 * 
 * If stdout is going to a file, the function verifies that a filename is present.
 * 
 * @param pipe_out: true/false whether command writes to pipe 
 * @param redir_type: type of file redirection commmand holds
 * @param out_fname: name of file for redirection
 * @return true or false
 */ 
bool is_valid_stdout(int pipe_out, enum redirect_type_e redir_type, char *out_fname) {
    // stdout can only be redirected once
    if (pipe_out && (redir_type == FILE_OUT_OVERWRITE || redir_type == FILE_OUT_APPEND)) {
        return ERROR;
    }

    // if rediction in but no filename, error
    if ((redir_type == FILE_OUT_OVERWRITE || redir_type == FILE_OUT_APPEND) && out_fname == NULL) {
        return ERROR;
    }

    return SUCCESS;
}


/**
 * Verifies that the command struct is valid by verifying that input and output channels 
 * are only redirected once or not at all. This means that if a command is piping out, it 
 * cannot also redirect out to a file. If a command is reading from a pipe, it cannot also
 * redirect a files contents in.
 * 
 * @param command: complete command struct to validate
 * @return true or false if commands stdin and stdout are valid
 */ 
int is_valid_command(struct command_t *command) {
    int rc;

    // check stdin configuration
    rc = is_valid_stdin(command->pipe_in, command->file_in, command->infile);
    if (rc < 0) return ERROR;

    // check stdout configuration
    rc = is_valid_stdout(command->pipe_out, command->file_out, command->outfile);
    if (rc < 0) return ERROR;

    return SUCCESS;
}


/**
 * Sets commands redirection out fields. First checks if this redirection was already specified and if so we
 * have a malformed command, return error. Otherwise the command struct is updated with the appropriate identifiers
 * 
 * @param command: structure to hold command representation
 * @param token: token which holds the file associated with the redirection
 * @param token_type: type of redirection 
 * @return true if set, false if redirection out already set and command is malformed
 */ 
int set_redirection_out(struct command_t *command, struct token_t *token, enum redirect_type_e token_type) {
    // if redir out already set, error
    if (command->file_out != REDIRECT_NONE) {
        return ERROR;
    }

    command->outfile = strdup(token->token_text);
    command->file_out = token_type;

    return SUCCESS;
}


/**
 * Sets commands redirection in fields. First checks if this redirection was already specified and if so we
 * have a malformed command, return error. Otherwise the command struct is updated with the appropriate identifiers
 * 
 * @param command: structure to hold command representation
 * @param token: token which holds the file associated with the redirection
 * @param token_type: type of redirection 
 * @return true if set, false if redirection in already set and command is malformed
 */ 
int set_redirection_in(struct command_t *command, struct token_t *token, enum redirect_type_e token_type) {
    // if redir in already set, error
    if (command->file_in != REDIRECT_NONE) {
        return ERROR;
    }

    command->infile = strdup(token->token_text);
    command->file_in = token_type;

    return SUCCESS;
}


/**
 * Iterates over all tokens to determine if redirection is present. If redirection is
 * found, updates the command structure accordingly to describe what kind of redirection
 * to handle and the corresponding filename that was specified for the redirection.
 * 
 * @param command: structure to hold command representation
 * @param head: head of the linked list of tokens
 * @return true if redirection set properly, false if redirection channel was already set and command is malformed
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
            else if (token_type == TOKEN_FNAME_OUT_APPEND) {
                rc = set_redirection_out(command, token, FILE_OUT_APPEND);
            }
            // <
            else if (token_type == TOKEN_FNAME_IN) {
                rc = set_redirection_in(command, token, FILE_IN);
            }

            // if any errors, return error
            if (rc < 0) return ERROR;

            // update current node after deleting the filename node
            struct list_head *next = curr->next;
            remove_token_node(curr, token);
            curr = next->prev; 
        }
    }

    return SUCCESS; 
}


/**
 * Converts list of tokens to a null terminated array of tokens and store in command
 * struct. Sets the command struct values of executable name and number of tokens
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
    command->num_tokens = size;
    command->cmd_name = command->tokens[0];
}


/**
 * Takes a list of tokens which describe a single command and creates a data structure
 * to represent all the information needed about the command.
 * 
 * @param command: structure to hold command representation
 * @param head: linked list hold tokens of the command
 * @param command_position: position of the command in the command line input
 * @param num_commands: number of commands present in the command line input
 * @return whether the conversion to command structure of successful or not
 */ 
int tokens_to_command(struct command_t *command, struct list_head *head, int command_position, int num_commands) {
    int rc; 

    // Initialize no file input/output
    command->file_in = REDIRECT_NONE;
    command->infile = NULL;
    command->file_out = REDIRECT_NONE;
    command->outfile = NULL;

    // set command pipe values
    int pipe_in = (command_position != 0) ? true : false;                  // if command is not first, pipe in
    int pipe_out = (command_position != num_commands - 1) ? true : false;  // if command is not last, pipe out
    command->pipe_in = pipe_in;
    command->pipe_out = pipe_out;

    // set command file params and check return code if command was malformed
    rc = set_command_redirections(command, head);
    if (rc < 0) return ERROR;

    // set command tokens array
    set_command_tokens(command, head);

    return SUCCESS;
}


/**
 * Once the list of tokens is created, this function checks if any redirection is present. If found,
 * the function determines if a filename was provided and what kind of operation should be done on
 * the file (overwrite, append, read). The operation type is stored in the corresponding file token
 * and the redirection token is deleted from the list since it is no longer needed.
 * 
 * @param head: linked list hold tokens of the command
 * @return status which descibes if all redirection was valid
 */ 
int is_tokens_redirection_valid(struct list_head *head) {
    struct list_head *curr;
    struct token_t *token;

    // Loop through all of the tokens to remove redirection tokens
    for (curr = head->next; curr != head; curr = curr->next) {
        token = list_entry(curr, struct token_t, list);
        char *tokstr = token->token_text;

        // if token text is redirection symbol
        if (is_redirection_text(tokstr)) {
            // redirection should not be last node is list
            if (curr->next == head) {
                return ERROR;
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
                curr = next->prev;
            }
        }
    }

    return SUCCESS;
}


/**
 * Handler when redirection character is encountered by statemachine. This function
 * handles all cases in which a redirection can appear
 *   - no space before or after redirection
 *   - 1 space before or 1 space after redirection
 *   - spaces on both sides of redirection
 * 
 * Function extracts redirection token, adds it to token linked list and updates
 * the statemachines next position so that it never sees a redirection character.
 * 
 * @param sm: state_machine struct
 * @param c: redirection character
 * @param list_tokens: linked list to hold the subcommand being parsed
 * @param cmdline: command input from user
 */ 
void parse_redirection_token(struct state_machine_t *sm, char c, struct list_head *list_tokens, char *cmdline) {
    // if redirection is found and substring is in statemachine, add substring to list (no space before redirection)
    if (sm->sub_len != 0) {
        // add token that precedes redirection to list (ex. cmd [word]> file ) 
        char *text = sub_string(cmdline, sm->sub_start, sm->sub_len);
        add_token_node(sm, list_tokens, text);
    }
   
    // redirection out 
    if (c == '>') {
        char next_char = cmdline[sm->position + 1];

        // check if redirection is >> by checking next char in string
        if (next_char == '>') {
            char *redir_tok = sub_string(cmdline, sm->position, 2);
            add_token_node(sm, list_tokens, redir_tok);
            sm->position++; // skip a char since we already added it to list
        } 
        // just a single redirection > so add it to list
        else {
            char *redir_tok = sub_string(cmdline, sm->position, 1);
            add_token_node(sm, list_tokens, redir_tok);
        }

    }
    // redirection in 
    else {
        // just a single redirection < so add it to list
        char *redir_tok = sub_string(cmdline, sm->position, 1);
        add_token_node(sm, list_tokens, redir_tok);
    }
}


/**
 * Handler when statemachine is in WHITESPACE state. Checks if the current character
 * is a quote or character - sets state accordingly. If it is neither a character or
 * quote, we encountered another whitespace and the handler does nothing.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 */ 
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
 * newline, we encountered another character. If character is a redirection, the function
 * handles distinguishing the redirection from another token. If not redirection, increment 
 * the substring length and continue.
 * 
 * @param sm: state_machine struct
 * @param c: character the statemachine is reading
 * @param list_tokens: linked list to hold the subcommand being parsed
 * @param cmdline: command input from user
 */ 
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
    else{
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
 */ 
void do_quote(struct state_machine_t *sm, char c, struct list_head *list_tokens, char *cmdline) {
    // if not quote or end of input, increment substring length
    if (c != '"' && c != '\0') {
        sm->sub_len++;
    } 
    // we reached the end of quoted substring
    else if (c == '"') {
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
*/
void tokenizer(struct state_machine_t *sm, struct list_head *list_tokens, char *cmdline) {
    // initialize statemachine
    initialize_statemachine(sm, cmdline);

    // initialize statemachine
    for (sm->position = 0; sm->position < sm->cmd_len; sm->position++) {
        // get character the statemachine is looking at
        char c = cmdline[sm->position];

        // if redirection and not between quotes, parse out
        if (sm->state != QUOTE && (c == '<' || c == '>')) {
            parse_redirection_token(sm, c, list_tokens, cmdline);
        }
        // not redirection or within quotes
        else {
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
    }

    // Once state machine reaches end, if not in WHITESPACE state then a last substring remains
    if (sm->state != WHITESPACE) {
        char *value = sub_string(cmdline, sm->sub_start, sm->position);
        add_token_node(sm, list_tokens, value);
    }
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
 */ 
int parse_command(struct command_t *commands_arr[], int num_commands, char *cmdline) {
    int rc;

    // split command line by pipes and store sub commands in array
    char **subcommands_arr = malloc(sizeof(char *) * num_commands); 
    split_cmdline(subcommands_arr, cmdline);

    // create statemachine for parser
    struct state_machine_t *sm = malloc(sizeof(struct state_machine_t));

    // parse each subcommands
    for (int i = 0; i < num_commands; i++) {
        // Initialize token list for sub command
        LIST_HEAD(list_tokens);


        // build list of tokens and verify redirection is valid
        tokenizer(sm, &list_tokens, subcommands_arr[i]);
        rc = is_tokens_redirection_valid(&list_tokens);
        if (rc < 0) return ERROR;


        // translate token list to subcommand structure
        struct command_t *command = malloc(sizeof(struct command_t));
        rc = tokens_to_command(command, &list_tokens, i, num_commands);
        if (rc < 0) return ERROR;
        

        // verifies stdout and stdin channels are valid
        rc = is_valid_command(command);
        if (rc < 0) return ERROR;


        // command is built and valid, add to array of commands
        commands_arr[i] = command;

        // free token list for parsed subcommand before next iteration
        clear_list(&list_tokens);
    }

    parser_clean_up(sm, subcommands_arr, num_commands);

    return SUCCESS;
}