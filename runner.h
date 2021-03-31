/**
 * @file: runner.h
 * @author: Michael Permyashkin
 * @author: Andrew Kress
 * 
 * @brief: Header file to define shared functions and structures across the shell. 
 * 
 * For consistent error checking, this header file defines a success and error return value which 
 * is used by all function in the shell that peform function that if fail, need to ripple through 
 * the shell to display or handle the error accordingly. 
 */ 

#include <stddef.h>
#include <stdbool.h>

#include "list.h"

#ifndef RUNNER_H
#define RUNNER_H

// constants for consistent error and success return values 
#define ERROR -1
#define SUCCESS 1

// constant sent to the shell when the internal exit command has been called and the input loop needs to terminate
#define EXIT_SHELL 999

// Definition of all types of redirection
enum redirect_type_e {
    REDIRECT_NONE,
    FILE_IN,
    FILE_OUT_OVERWRITE,
    FILE_OUT_APPEND
};

// The command data structure which holds all information needed by the shell to execute the command
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
 * Utility function to get the substring given a start index and length. Returns to substring.
 * 
 * @param str - The source string you want a substring from
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * @return extracted substring
 */
char * sub_string(char* str, int start, int length);

/**
 * Called by the main shell loop each time input is recieved. This function handles parsing the 
 * command line into an array of commands ready for execution. The commands are then directed 
 * to the execution unit appropraite for the command that was given. As the command line input
 * moves through the shell, converted to command data structures and is executed, any errors will 
 * cause a return value to be sent back to this function which will then handle the error and return 
 * back to the main loop of the shell.
 * 
 * @param cmdline: the command that was entered by user
 * @return: status of command(s) execution
 */ 
int do_command(char *cmdline);

/**
 * Takes the command line input which may contain many commands seperated by a pipe. The command line
 * input is seperated into an array of subcommands and each is seperated into a list of tokens. The list
 * of tokens are then converted into a data structure which represents a command that will be executed by
 * the shell and holds all information needed by the execution unit. The parser populates the given empty 
 * array with the complete and valid command structures.
 * 
 * If any command line input is invalid, an error message is printed to the console and the shell returns
 * to the prompt.
 * 
 * @param commands_arr: array to hold command stucts
 * @param num_commands: number of subcommands to parse
 * @param cmdline: the command line given by the user that will be parsed
 * @return status of command line parsing
 **/ 
int parse_command(struct command_t *commands_arr[], int num_commands, char *cmdline);


#endif