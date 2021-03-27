#include <stdbool.h>
#include "cmdline.h"

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