#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "environ.h"
#include "internal.h"
#include "cmdline.h"

int main(int argc, char **argv, char **envp) {
    environ_init(envp);

    struct command_t *cmd = malloc(sizeof(struct command_t));
    cmd->cmd_name = "boop";
    cmd->num_tokens = 2;
    char *toks[] = { "boop", NULL };
    cmd->tokens = toks;

    is_internal_command(cmd);

    environ_clean_up();
    free(cmd);
}