#include "cmdline.h"
#include <stdbool.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "environ.h"
#include <dirent.h>

struct internal_command_t {
    char *name;
    int argc;
    int (*handler)(struct command_t *cmd);
};

int handle_setenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 2) {
        environ_set_var(cmd->tokens[1], cmd->tokens[2]);
    } else {
        LOG_ERROR(ERROR_SETENV_ARG);
    }
    return 0;
}

int handle_getenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        char **envp = make_environ();
        for (int i = 0; envp[i] != NULL; i++) {
            printf("%s\n", envp[i]);
        }
    } else if (cmd->num_tokens - 2 == 1) {
        if (environ_var_exist(cmd->tokens[1])) {
            printf("%s\n", environ_get_var(cmd->tokens[1])->value);
        } else {
            LOG_ERROR(ERROR_GETENV_INVALID, cmd->tokens[1]);
        }
    } else {
        LOG_ERROR(ERROR_GETENV_ARG);
    }
    return 0;
}

int handle_unsetenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 1) {
        if (environ_var_exist(cmd->tokens[1]))
            environ_remove_var(cmd->tokens[1]);
    } else {
        LOG_ERROR(ERROR_UNSETENV_ARG);
    }
    return 0;
}

int handle_cd(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        environ_set_var("PWD", "~");
    } else if (cmd->num_tokens - 2 == 1) {
        DIR* dir = opendir("mydir");
        if (dir) {
            closedir(dir);
            environ_set_var("PWD", cmd->tokens[1]);
        } else {
            LOG_ERROR(ERROR_CD_NOHOME);
        }
    } else {
        LOG_ERROR(ERROR_CD_ARG);
    }
    return 0;
}

int handle_pwd(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        if (environ_var_exist("PWD")) {
            printf("%s\n", environ_get_var("PWD")->value);
        }
    } else {
        LOG_ERROR(ERROR_PWD_ARG);
    }
    return 0;
}

int handle_exit(struct command_t *cmd) {
    if (cmd->num_tokens - 2 != 0) {
        LOG_ERROR(ERROR_EXIT_ARG);
    }
    return 0;
}

struct internal_command_t internal_cmds[] = {
    { .name = "setenv", .handler = handle_setenv },
    { .name = "getenv", .handler = handle_getenv },
    { .name = "unsetenv", .argc = 1, .handler = handle_unsetenv },
    { .name = "cd", .handler = handle_cd },
    { .name = "pwd", .handler = handle_pwd },
    { .name = "exit", .handler = handle_exit },
    NULL
};

bool is_internal_command(struct command_t *cmd) {
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        if (strcmp(internal_cmds[i].name, cmd->cmd_name) == 0)
            return true;
    }
    return false;
}

int execute_internal_command(struct command_t *cmd) {
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        if (strcmp(internal_cmds[i].name, cmd->cmd_name) == 0)
            return internal_cmds[i].handler(cmd);
    }
}