/**
 * @file: internal.c
 * @author: Andrew Kress
 * 
 * @brief: Handles and runs the internal shell commands.
 * 
 * When execute_internal_command is called, it loops through
 * the available commands and runs the handlers for the
 * corresponding command.
 */ 

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmdline.h"
#include "internal.h"
#include "error.h"
#include "environ.h"
#include "background.h"

#define ARGC_OFFSET 2

struct internal_command_t {
    char *name;
    int (*handler)(struct command_t *cmd);
};

/**
 * Handles the setenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_setenv(struct command_t *cmd) {
    // If there are two args,
    // set environment variable with the name of
    // arg1 to the value of arg2.
    if (cmd->num_tokens - ARGC_OFFSET == 2) {
        environ_set_var(cmd->tokens[1], cmd->tokens[2]);
    } else {
        // Print error if there aren't two args.
        LOG_ERROR(ERROR_SETENV_ARG);
    }
    return SUCCESS;
}

/**
 * Handles the getenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_getenv(struct command_t *cmd) {
    // If there aren't any args for getenv,
    // print the whole environment.
    if (cmd->num_tokens - ARGC_OFFSET == 0) {
        environ_print();
    // If there is one arg for getenv,
    // print the environment variable if it exists.
    } else if (cmd->num_tokens - ARGC_OFFSET == 1) {
        if (environ_var_exist(cmd->tokens[1])) {
            // Get the environment variable and print in the format
            // NAME=value
            struct environ_var_t *env_var = environ_get_var(cmd->tokens[1]);
            printf("%s=%s\n", env_var->name, env_var->value);
        } else {
            // Print an error if the variable doesn't exist
            LOG_ERROR(ERROR_GETENV_INVALID, cmd->tokens[1]);
        }
    } else {
        // Print an error if there are two or more arguments
        LOG_ERROR(ERROR_GETENV_ARG);
    }
    return SUCCESS;
}

/**
 * Handles the unsetenv internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_unsetenv(struct command_t *cmd) {
    // If there is one arg,
    // delete the environment variable, if it exists.
    if (cmd->num_tokens - ARGC_OFFSET == 1) {
        if (environ_var_exist(cmd->tokens[1]))
            environ_remove_var(cmd->tokens[1]);
    } else {
        // Print error if there isn't one arg.
        LOG_ERROR(ERROR_UNSETENV_ARG);
    }
    return SUCCESS;
}

/**
 * Handles the cd internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_cd(struct command_t *cmd) {
    // If there aren't an args,
    // chdir to the HOME variable if it exists.
    if (cmd->num_tokens - ARGC_OFFSET == 0) {
        if (environ_var_exist("HOME")) {
            chdir(environ_get_var("HOME")->value);
        } else {
            // Print error if HOME doesn't exist.
            LOG_ERROR(ERROR_CD_NOHOME);
        }
    // If there is one arg,
    // chdir to the new directory.
    } else if (cmd->num_tokens - ARGC_OFFSET == 1) {
        chdir(cmd->tokens[1]);
        char *cwd = malloc(1024);
        getcwd(cwd, 1024);
        environ_set_var("PWD", cwd);
    } else {
        // Print error if there are two or more args.
        LOG_ERROR(ERROR_CD_ARG);
    }
    return SUCCESS;
}

/**
 * Handles the pwd internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_pwd(struct command_t *cmd) {
    // If there aren't any args,
    // get cwd and print it.
    if (cmd->num_tokens - ARGC_OFFSET == 0) {
        // Get cwd.
        char *cwd = malloc(1024);
        getcwd(cwd, 1024);
        // Print cwd.
        printf("%s\n", cwd);
    } else {
        // Print error if there is one or more args
        LOG_ERROR(ERROR_PWD_ARG);
    }
    return SUCCESS;
}

/**
 * Handles the exit internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_exit(struct command_t *cmd) {
    // If there are any arguments,
    // print error.
    if (cmd->num_tokens - ARGC_OFFSET != 0) {
        LOG_ERROR(ERROR_EXIT_ARG);
        return ERROR;
    }
    // Return shell exit code.
    return EXIT_SHELL;
}

/**
 * Handles the queue internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_queue(struct command_t *cmd) {
    int rc;
    if (is_valid_background_command(cmd)) {

        // remove first token which is the internal command `queue`
        for(int i=1; i<cmd->num_tokens; i++)
            cmd->tokens[i-1] = cmd->tokens[i];

        cmd->num_tokens = cmd->num_tokens-1;
        cmd->cmd_name = cmd->tokens[0];

        // set background commands stdin and stdout
        rc = set_command_channels(cmd);
        if (rc < 0) return ERROR;

        add_to_queue(cmd);
    }
}

/**
 * Handles the status internal command
 * 
 * @param cmd - The command for arguments
 */
int handle_status(struct command_t *cmd) {
    print_all_job_status();
    return SUCCESS;
}

int handle_output(struct command_t *cmd) {
    int job_id = atoi(cmd->tokens[1]);
    print_job_output(job_id);
}

int handle_cancel(struct command_t *cmd) {
    int job_id = atoi(cmd->tokens[1]);
    print_job_output(job_id);
}


/**
 * The array of available internal commands.
 */
struct internal_command_t internal_cmds[] = {
    { .name = "setenv", .handler = handle_setenv },
    { .name = "getenv", .handler = handle_getenv },
    { .name = "unsetenv", .handler = handle_unsetenv },
    { .name = "cd", .handler = handle_cd },
    { .name = "pwd", .handler = handle_pwd },
    { .name = "exit", .handler = handle_exit },
    { .name = "queue", .handler = handle_queue },
    { .name = "status", .handler = handle_status },
    { .name = "output", .handler = handle_output },
    { .name = "cancel", .handler = handle_cancel },
    NULL
};

/**
 * Checks if the given command is an internal command
 * 
 * @param cmd - The command to check
 */
bool is_internal_command(struct command_t *cmd) {
    // Loop through the internal commands,
    // comparing their names to the name of cmd.
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        // If the names match, return true.
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
    // Loop through the internal commands,
    // comparing their name to the name of cmd.
    for (int i = 0; internal_cmds[i].name != NULL; i++) {
        // If the names match, return the result of the
        // corresponding command handler.
        if (strcmp(internal_cmds[i].name, cmd->cmd_name) == 0)
            return internal_cmds[i].handler(cmd);
    }
    return ERROR;
}