/**
 * @file: executor.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file for functions needed by the shell to use the execution unit
 */ 
#include <stddef.h>

#include "runner.h"

/**
 * Driver function which executes an array of commands using the information stored in each command 
 * stuct to determine the behavior of each commands execution.
 * 
 * @param commands_arr: array of command structs to execute
 * @param num_commands: the number of commands to execute
 * 
 * @return status of all setup tasks and all commands execution
 */ 
int execute_external_command(struct command_t *commands_arr[], int num_commands);