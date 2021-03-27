#include "cmdline.h"
#include <stdbool.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "environ.h"
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

struct internal_command_t {
    char *name;
    int argc;
    int (*handler)(struct command_t *cmd);
};

/**
 * Handles the setenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_setenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 2) {
        environ_set_var(cmd->tokens[1], cmd->tokens[2]);
    } else {
        LOG_ERROR(ERROR_SETENV_ARG);
    }
    return 0;
}

/**
 * Handles the getenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_getenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        environ_print();
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

/**
 * Handles the unsetenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_unsetenv(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 1) {
        if (environ_var_exist(cmd->tokens[1]))
            environ_remove_var(cmd->tokens[1]);
    } else {
        LOG_ERROR(ERROR_UNSETENV_ARG);
    }
    return 0;
}

/**
 * Handles the cd internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_cd(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        if (environ_var_exist("HOME")) {
            int ec = chdir(environ_get_var("HOME")->value);
            if (ec != 0) {
                printf("ERROR: %d\n", ec);
            }
        } else {
            LOG_ERROR(ERROR_CD_NOHOME);
        }
    } else if (cmd->num_tokens - 2 == 1) {
        chdir(cmd->tokens[1]);
    } else {
        LOG_ERROR(ERROR_CD_ARG);
    }
    return 0;
}

/**
 * Handles the pwd internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_pwd(struct command_t *cmd) {
    if (cmd->num_tokens - 2 == 0) {
        char *cwd = malloc(1024);
        getcwd(cwd, 1024);
        printf("%s\n", cwd);
    } else {
        LOG_ERROR(ERROR_PWD_ARG);
    }
    return 0;
}

/**
 * Handles the exit internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_exit(struct command_t *cmd) {
    if (cmd->num_tokens - 2 != 0) {
        LOG_ERROR(ERROR_EXIT_ARG);
    }
    
    environ_clean_up();
    exit(0);
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

/**
 * Checks if the given command is an internal command
 * 
 * @param cmd - The command to check
 */
bool is_internal_command(struct command_t *cmd) {
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        if (strcmp(internal_cmds[i].name, cmd->cmd_name) == 0)
            return true;
    }
    return false;
}

/**
 * Executed the given internal command
 * 
 * @param cmd - The command for arguments
 */
int execute_internal_command(struct command_t *cmd) {
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        if (strcmp(internal_cmds[i].name, cmd->cmd_name) == 0)
            return internal_cmds[i].handler(cmd);
    }
    return 0;
}