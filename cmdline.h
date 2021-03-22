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
 */
enum token_types_e {
    TOKEN_NORMAL,
    TOKEN_REDIR,
    TOKEN_FNAME_IN,
    TOKEN_FNAME_OUT_OVERWRITE,
    TOKEN_FNAME_OUT_APPEND,
};


/**
 * Definition of all types of redirection
 */
enum redirect_type_e {
    REDIRECT_NONE,
    FILE_IN,
    FILE_OUT_OVERWRITE,
    FILE_OUT_APPEND
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


/**
 * The command datastructure which holds all information needed by the shell to execute the command
 * 
 * @num_tokens: number of tokens that were parsed
 * @tokens: array of token strings
 * 
 * @pipe_in: command reads from pipe
 * @pipe_out: command writes to pipe
 * 
 * @file_in: command writes to file
 * @infile: name of file to write to 
 * 
 * @file_out: command reads from file
 * @outfile: name of file to read from
 * 
 * @list: head of 
 **/ 
struct command_t {
    int num_tokens;
    char **tokens;

    int pipe_in;
    int pipe_out;

    enum redirect_type_e file_in;
    char *infile;

    enum redirect_type_e file_out;
    char *outfile;
};


/**
 * Takes the command line input, parses the command and executes the array of commands
 * 
 * @param cmdline: the command that was entered by user
 * 
 * @return: status of command(s) execution
 */ 
int do_command(char *cmdline);


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
int parse_command(struct command_t *commands_arr[], int num_commands, char *cmdline);

#endif