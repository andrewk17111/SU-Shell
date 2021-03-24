#include <stdbool.h>

// check if command is in internal cmd table
bool is_internal_command(struct command_t *cmd);


// executes internal command
int execute_internal_command(struct command_t *cmd);