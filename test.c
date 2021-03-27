#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "list.h"
#include "environ.h"
#include "internal.h"
#include "cmdline.h"

int main(int argc, char **argv, char **envp) {
    environ_init(envp);

    struct command_t *cmd = malloc(sizeof(struct command_t));
    cmd->cmd_name = "cd";
    cmd->num_tokens = 3;
    char *toks[] = { "cd", "..", NULL };
    cmd->tokens = toks;

    execute_internal_command(cmd);

    cmd->cmd_name = "pwd";
    cmd->num_tokens = 2;
    char *toks2[] = { "pwd", NULL };
    cmd->tokens = toks2;

    execute_internal_command(cmd);

    environ_clean_up();
    free(cmd);
}