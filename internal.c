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

#include "runner.h"
#include "internal.h"
#include "error.h"
#include "environ.h"
#include "background.h"


// argc offset set to 2 because tokens array include executable name and null
#define ARGC_OFFSET 2


// internal command struct holds name of command and handler when that command is called
struct internal_command_t {
    char *name;
    int (*handler)(struct command_t *cmd);
};


/**
 * Handles the setenv command to set the value of an existing
 * environment variable or add a new environment variable.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
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
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the getenv command to get the value of an
 * environment variable or get the value of all the variables.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
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
            return ERROR;
        }
    } else {
        // Print an error if there are two or more arguments
        LOG_ERROR(ERROR_GETENV_ARG);
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the unsetenv command to remove a variable from
 * the internal environment.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
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
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the cd command to change the current working
 * directory of sush.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
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
            return ERROR;
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
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the pwd command to print the current
 * working directory.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
 */
int handle_pwd(struct command_t *cmd) {
    // If there aren't any args,
    // get cwd and print it.
    if (cmd->num_tokens - ARGC_OFFSET == 0) {
        // Get cwd.
        char *cwd = malloc(1024);
        getcwd(cwd, 1024);
        // Print the current working directory.
        printf("%s\n", cwd);
    } else {
        // Print error if there is one or more args
        LOG_ERROR(ERROR_PWD_ARG);
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the exit command to terminate sush.
 * 
 * @param cmd - The command for arguments
 * 
 * @return EXIT_SHELL or ERROR if the command succeeds or fails.
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
 * Handles the queue command to add the given command to
 * the background job queue.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
 */
int handle_queue(struct command_t *cmd) {
    int rc;

    if (cmd->num_tokens - ARGC_OFFSET > 1) {

        // checks that stdin and stdout are not being changed
        if (is_valid_background_command(cmd)) {
            // free token `queue` from token array and cmd_name field
            free(cmd->tokens[0]);

            // remove first token which is the internal command `queue`
            for(int i=1; i<cmd->num_tokens; i++) {
                cmd->tokens[i-1] = cmd->tokens[i];
            }

            cmd->num_tokens = cmd->num_tokens-1;
            cmd->cmd_name = cmd->tokens[0];

            // set background commands stdin and stdout
            rc = set_command_channels(cmd);
            if (rc < 0) return ERROR;

            add_to_queue(cmd);
        } 
    } else {
        LOG_ERROR(ERROR_QUEUE_ARG);
        return ERROR;
    }
}


/**
 * Handles the status command to get the statuses of the
 * jobs in the job queue.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
 */
int handle_status(struct command_t *cmd) {
    // If there aren't any args,
    // print the statuses of the jobs.
    if (cmd->num_tokens - ARGC_OFFSET == 0) {
        print_all_job_status();
    } else {
        // Print error if there is one or more args
        LOG_ERROR(ERROR_STATUS_ARG);
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the output command to get the output of
 * the requested background job.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
 */
int handle_output(struct command_t *cmd) {
    // If there is one arg,
    // print the output of the requested job.
    if (cmd->num_tokens - ARGC_OFFSET == 1) {
        int job_id = atoi(cmd->tokens[1]);
        print_output_and_remove(job_id);
    } else {
        // Print error if there is one or more args
        LOG_ERROR(ERROR_OUTPUT_ARG);
        return ERROR;
    }
    return SUCCESS;
}


/**
 * Handles the cancel command to cancel a
 * background job.
 * 
 * @param cmd - The command for arguments
 * 
 * @return SUCCESS or ERROR if the command succeeds or fails.
 */
int handle_cancel(struct command_t *cmd) {
    // If there is one arg,
    // remove the job from the queue.
    if (cmd->num_tokens - ARGC_OFFSET == 1) {
        int job_id = atoi(cmd->tokens[1]);
        attempt_cancel_command(job_id);
    } else {
        // Print error if there is one or more args
        LOG_ERROR(ERROR_CANCEL_ARG);
        return ERROR;
    }
    return SUCCESS;
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
 * Checks if the given command is an internal command.
 * 
 * @param cmd - The command to check
 * 
 * @return True if the command was found. False if it wasn't.
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
 * Executed the given internal command.
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