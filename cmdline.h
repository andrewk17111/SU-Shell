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

// constants for code readability for parser functionality
#define TRUE 1
#define FALSE 0 

#define RETURN_ERROR -1 
#define RETURN_SUCCESS 0 



// Definition of all types of redirection
enum redirect_type_e {
    REDIRECT_NONE,
    FILE_IN,
    FILE_OUT_OVERWRITE,
    FILE_OUT_APPEND
};


/**
 * The command data structure which holds all information needed by the shell to execute the command
 * 
 * @cmd_name: name of executable
 * @tokens: array of token strings
 * @num_tokens: number of tokens that were parsed
 * 
 * @pipe_in: does command read from pipe
 * @pipe_out: does command write to pipe
 * 
 * @file_in: does command write to file
 * @infile: name of file to write to 
 * 
 * @file_out: does command read from file
 * @outfile: name of file to read from
 **/ 
struct command_t {
    char *cmd_name;
    char **tokens;
    int num_tokens;

    int pipe_in;
    int pipe_out;

    enum redirect_type_e file_in;
    char *infile;
    int fid_in;

    enum redirect_type_e file_out;
    char *outfile;
    int fid_out;
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
 * @return status of command line parsing
 **/ 
int parse_command(struct command_t *commands_arr[], int num_commands, char *cmdline);

#endif