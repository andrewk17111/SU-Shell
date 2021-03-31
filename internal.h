/**
 * @file: internal.h
 * @author: Andrew Kress
 * 
 * @brief: Header file for internal commands
 * 
 * Defines functions for checking if a command is internal
 * and executing and internal command.
 */ 

#include <stdbool.h>

#include "runner.h"

#ifndef INTERNAL_H
#define INTERNAL_H

/**
 * Checks if the given command is an internal command
 * 
 * @param cmd - The command to check
 */
bool is_internal_command(struct command_t *cmd);

/**
 * Executed the given internal command
 * 
 * @param cmd - The command for arguments
 */
int execute_internal_command(struct command_t *cmd);

#endif