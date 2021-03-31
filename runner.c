/**
 * @file: runner.c
 * @author: Michael Permyashkin
 * @author: Andrew Kress
 * 
 * @brief: Interface to shell prompt to parse and execute commands
 * 
 * Shells prompts user for input and passes command line input to this
 * interface for processing. Function `do_command` drives processing by
 * calling the parser to build command data structures and then determining
 * which execution unit should handle the command(s).
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runner.h"
#include "error.h"
#include "internal.h"
#include "executor.h"
#include "environ.h"
#include "background.h"


/**
 * frees all allocated memory to hold command structures. Checks if command
 * is in queue before freeing the command structure to prevent a double free
 * when item is removed from queue.
 * 
 * @param commands_arr: array of command structures to free
 * @param num_commands: number of command structures present
 */ 
void runner_clean_up(struct command_t *commands_arr[], int num_commands) {
    for (int i=0; i<num_commands; i++) {

        // only free commands not in queue
        if (!is_command_in_queue(commands_arr[i])) {
            // free each token in array
            int num_toks = commands_arr[i]->num_tokens;
            for (int j=0;j<num_toks; j++ ) {
                free(commands_arr[i]->tokens[j]);
            }
            // free token array
            free(commands_arr[i]->tokens);

            // free filename fields
            free(commands_arr[i]->outfile);
            free(commands_arr[i]->infile);
            
            // free command struct
            free(commands_arr[i]);
        }
    }
}


/**
 * Counts the number of subcommands present in a given command line. Each time
 * a pipe is encountered we incremenet the counter and return the final value.
 * 
 * @param cmdline: the command that was entered by user
 * 
 * @return: interger value of the number of subcommands found
 */ 
int get_num_subcommands(char *cmdline) {
    int count = 1;
    int cmd_len = strlen(cmdline);

    for (int i=0; i<cmd_len; i++) {
        if (cmdline[i] == '|') {
            count++;
        }
    }
    return count;
}


/**
 * Takes the command line input, parses the command and executes the array of commands
 * 
 * @param cmdline: the command that was entered by user
 * 
 * @return: status of command(s) execution
 */ 
int do_command(char *cmdline) {
    int rc;

    // count number of commands and allocate memory to hold n command stucts
    int num_commands = get_num_subcommands(cmdline);
    struct command_t *commands_arr[num_commands];

    // parse commands to populate array of command structs
    rc = parse_command(commands_arr, num_commands, cmdline);
    if (rc < 0) {
        LOG_ERROR(ERROR_INVALID_CMDLINE);
        return rc;
    }

    // call respective execution unit
    if (is_internal_command(commands_arr[0])) {
        rc = execute_internal_command(commands_arr[0]);
    } else {
        rc = execute_external_command(commands_arr, num_commands);
    }

    // release all memory allocated to hold commands
    runner_clean_up(commands_arr, num_commands);

    return rc;
}