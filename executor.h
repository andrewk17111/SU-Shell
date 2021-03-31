/**
 * @file: executor.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file for functions needed by the shell to use the execution unit for
 * all non-internal commands
 * 
 * Defines a function that is the interface to execute any command(s) that are not 
 * internal commands.
 */ 
#include <stddef.h>

#include "runner.h"

#ifndef EXECUTOR_H
#define EXECUTOR_H

/**
 * Driver function which executes an array of commands using the information stored in 
 * each command stuct to determine the behavior of each commands execution. During each
 * commands setup, all return codes are error checked and execution will terminate if
 * any errors are encountered.
 * 
 * The function expects an array of command_t structs (defined in runner.h) and the number
 * of commands being executed. Consecutive commmands are pipelined together and each command
 * structure should hold values that reflect the pipelining behavior.
 * 
 * @param commands_arr: array of command structs to execute
 * @param num_commands: the number of commands to execute
 * @return status of all setup tasks and all commands execution
 */ 
int execute_external_command(struct command_t *commands_arr[], int num_commands);

#endif