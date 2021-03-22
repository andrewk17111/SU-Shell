
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmdline.h"
#include "error.h"


void print_command_struct(struct command_t *command) {
    printf("*********************************\n");

    // ->num_tokens
    printf("    num_tokens -> %d\n", command->num_tokens);
    
    // ->tokens
    printf("    tokens -> ");
    for (int i = 0; i < command->num_tokens; i++) {
        printf("[%s] ", command->tokens[i]);
    }
    printf("\n");

    // ->file_in
    printf("    file_in -> %d\n", command->file_in);
    printf("    infile -> %s\n", command->infile);

    // ->file_out
    printf("    file_out -> %d\n", command->file_out);
    printf("    outfile -> %s\n", command->outfile);

    // ->pipe_in and pipe_out
    printf("    pipe_in -> %d\n", command->pipe_in);
    printf("    pipe_out -> %d\n", command->pipe_out);

    printf("*********************************\n");
}

void print_command_list(struct command_t *commands[], int num_cmds) {
    for (int i=0; i<num_cmds; i++) 
        print_command_struct(commands[i]);
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


void clean_up(struct command_t *commands_arr[], int num_commands) {
    for (int i=0; i<num_commands; i++) {
        
        int num_tokens = commands_arr[i]->num_tokens;
        for (int j=0; j<num_tokens; j++) {
            free(commands_arr[i]->tokens[j]);
        }
        free(commands_arr[i]->tokens);
        free(commands_arr[i]);
    }
    free(commands_arr);
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

    // split command line into array of subcommands
    int num_commands = get_num_subcommands(cmdline);

    // Create struct array to hold all command structures
    struct command_t *commands_arr[num_commands];

    rc = parse_command(commands_arr, num_commands, cmdline);
    if (rc < 0) return rc;

    print_command_list(commands_arr, num_commands);

    // clean_up(commands_arr, num_commands);

    return 0;
}