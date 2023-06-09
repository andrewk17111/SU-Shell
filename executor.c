/**
 * @file: executor.c
 * @author: Michael Permyashkin
 * 
 * @brief: Executes an array of commands
 * 
 * Execution unit of the shell to execute all non-internal commands. Takes an array of commands
 * and handles pipelining of stdin and stdout channels if necessary. Additional error checking
 * is present to ensure that any channel is only redirected once (i.e. cannot redirect out to file
 * and also write to a pipe) - this is checked elsewhere in the shell but done again here prior
 * to execution of any command.
 */ 
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "runner.h"
#include "executor.h"
#include "environ.h"
#include "error.h"


// constants for pipe code readability
#define READ_PIPE 0
#define WRITE_PIPE 1


/**
 * Resets stdin and stdout after command(s) execution to ensure
 * both are reset to default.
 * 
 * @param stdin_copy: default stdin 
 * @param stdout_copy: default stdout 
 * @return status of reset
 */ 
int reset_stdin_stdout(int stdin_copy, int stdout_copy) {
    int rc;
    
    // reset stdin
    rc = dup2(stdin_copy, STDIN_FILENO);
    if (rc < 0) return ERROR;
    
    // reset stdout
    rc = dup2(stdout_copy, STDOUT_FILENO);
    if (rc < 0) return ERROR;
    
    rc = close(stdin_copy);
    if (rc < 0) return ERROR;

    rc = close(stdout_copy);
    if (rc < 0) return ERROR;

    return true;
}


/**
 * Creates/opens file for redirection out. Takes the name of the file and the type
 * of redirection to be done (overwrite or append). If overwriting, the file is
 * created if it does not exist or truncates the existing file. If appending, the file 
 * is created if it does not exist, or opens it for appending.
 * 
 * @param fname: name of the file
 * @param redir_type: type of redirection out to be done 
 * @return fid will be positive if opened/created successully, otherwise negative
 */ 
int create_out_file(char *fname, enum redirect_type_e redir_type) {
    int fid;

    // if create/overwrite
    if (redir_type == FILE_OUT_OVERWRITE) {
        fid = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);
        // error, unable to open file
        if (fid < 0) {
            LOG_ERROR(ERROR_EXEC_OUTFILE, strerror(errno));
        }
    }
    // if append
    else if (redir_type == FILE_OUT_APPEND) {
        fid = open(fname, O_RDWR | O_CREAT | O_APPEND, 0777);
        // error, unable to open file
        if (fid < 0) {
            LOG_ERROR(ERROR_EXEC_APPEND, strerror(errno));
        }
    }

    return fid;
}


/**
 * Opens file for redirection in. If the filename exists, the file is opened for
 * read and returns the file id. If the file does not 
 * 
 * @param fname: name of the file
 * @param redir_type: type of redirection out to be done 
 * @return fid will be positive if opened successully, otherwise negative
 */ 
int open_in_file(char *fname) {
    int fid = open(fname, O_RDONLY, 0777);

    // error, unable to open file
    if (fid < 0) {
        LOG_ERROR(ERROR_EXEC_INFILE, strerror(errno));
    }

    return fid;
}


/**
 * Creates and stores appropriate file id's in the command struct
 * for any redirection that is defined.
 * 
 * @param command: command to check for any redirection
 * @return status of redirection file setup 
 */ 
int setup_command_redirection(struct command_t *command) {
    int fid;

    // open file if fid not yet retrieved for stdin
    if (command->file_out && command->fid_out == 0) {
        fid = create_out_file(command->outfile, command->file_out);
        if (fid < 0) return ERROR;

        // output file created/opened for write
        command->fid_out = fid; 
    }

    // open file if fid not yet retrieved for stdout
    if (command->file_in && command->fid_in == 0) {
        fid = open_in_file(command->infile);
        if (fid < 0) return ERROR;

        // input file opened for read
        command->fid_in = fid;
    }

    return SUCCESS;
}


/**
 * Sets stdin based on command information. Stdin will either be from a file
 * whose fid is stored in struct, a pipes read side, or the default
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_out: write side of the pipe used if given command preceeds another
 * @return status of attempting to set stdout channels 
 */ 
int set_stdout(struct command_t *command, int pipe_out) {
    int rc;

    // writing to file
    if (command->file_out) {
        // close stdout
        rc = close(STDOUT_FILENO);
        if (rc < 0) return ERROR;

        // attach fid to stdout
        rc = dup(command->fid_out);
        if (rc < 0) return ERROR;
    } 
    // writing to pipe
    else if (command->pipe_out) {
        // close stdout
        rc = close(STDOUT_FILENO);
        if (rc < 0) return ERROR;

        // attach pipe to stdout
        rc = dup2(pipe_out, STDOUT_FILENO);
        if (rc < 0) return ERROR;
    }
    // else keep default
    return SUCCESS;
}


/**
 * Sets stdin based on command information. Stdin will either be from a file
 * whose fid is stored in struct, a pipes read side, or the default
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_in: read side of the pipe used if given command proceeds another
 * @return status of attempting to set stdin channels 
 */ 
int set_stdin(struct command_t *command, int pipe_in) {
    int rc;
    
    // reading from file
    if (command->file_in) {
        rc = close(STDIN_FILENO);
        if (rc < 0) return ERROR;
       
        rc = dup(command->fid_in);
        if (rc < 0) return ERROR;
    } 
    // reading from pipe
    else if (command->pipe_in) {
        // you are not the last command -> stdin from pipes
        rc = close(STDIN_FILENO);
        if (rc < 0) return ERROR;

        rc = dup2(pipe_in, STDIN_FILENO);
        if (rc < 0) return ERROR;
    }
    // else keep default
    return SUCCESS;
}


/**
 * Sets stdin and stdout according to information stored in command struct which 
 * describes input and output destinations. After setup, the command is executed and
 * the function will only return if command execution fails.
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_in: read side of the pipe used if given command proceeds another
 * @param pipe_out: write side of the pipe used if given command preceeds another
 * @param envp: environement for command execution
 * @return will only return if exec fails
 **/ 
int do_child(struct command_t *command, int pipe_in, int pipe_out, char *const envp[]) {
    int rc;

    // set stdout channel
    rc = set_stdout(command, pipe_out);
    if (rc < 0) return ERROR;

    // set stdin channel
    rc = set_stdin(command, pipe_in);
    if (rc < 0) return ERROR;

    // execute command
    execvpe(command->cmd_name, command->tokens, envp);

    // if exec returns, something went wrong
    LOG_ERROR(ERROR_EXEC_FAILED, strerror(errno));
    exit(ERROR);
}


/**
 * The parent process waits for the child to finish
 * 
 * @param pid: id of the child process
 **/ 
void do_parent(int pid) {
    int status;
    waitpid(pid, &status, 0);
}


/**
 * Forks the process and calls functions to handle child and parent processes respectively.
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_in: read side of the pipe used if given command proceeds another
 * @param pipe_out: write side of the pipe used if given command preceeds another
 * @param envp: environement for command execution
 * @return status of command execution
 */ 
int fork_and_exec(struct command_t *command, int pipe_in, int pipe_out, char *const envp[]) {
    pid_t pid = fork();
    
    // check fork() return to ensure it is a valid pid, otherwise an error occured
    if (pid < 0) {
        return false;
    } 
    // child executes command
    else if (pid == 0) {
        do_child(command, pipe_in, pipe_out, envp);
    } 
    // parent waits for child to finish
    else {
        do_parent(pid);
    }

    return SUCCESS;
}


/**
 * Each command uses this function to first setup all redirection files, forks and
 * executes the command given.
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_in: read side of the pipe used if given command proceeds another
 * @param pipe_out: write side of the pipe used if given command preceeds another
 * @param envp: environement for command execution
 * @return status of setup and command execution
 */ 
int setup_and_execute_command(struct command_t *command, int pipe_in, int pipe_out, char *const envp[]) {
    int rc;

    // creates/opens any files to be used for command redirection
    rc = setup_command_redirection(command);
    if (rc < 0) return ERROR;

    rc = fork_and_exec(command, pipe_in, pipe_out, envp);
    if (rc < 0) return ERROR;

    return SUCCESS;
}


/**
 * Cleans up memory allocated to environment array used for execution.
 * Frees each element of the array and finishes by freeing the array memory
 * 
 * @param envp: environement array to free
 */ 
void executor_cleanup(char **envp) {
    int i = 0;
    while (envp[i] != NULL) {
        free(envp[i]);
        i++;
    }
    free(envp);
}


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
int execute_external_command(struct command_t *commands_arr[], int num_commands) {

    // store default stdin and stdout to reset after all execution
    int stdin_copy = dup(STDIN_FILENO);
    int stdout_copy = dup(STDOUT_FILENO);

    // build evnp for commands execution
    char **envp = make_environ();
    
    int i, rc;
    pid_t pid;

    // create pipes
    int pipes_fd[2];
    int pipe_in = 0;

    // fork and execute each command
    for (i=0; i<num_commands; i++) {
        // create pipe
        rc = pipe(pipes_fd);
        if (rc < 0) return ERROR;

        // use command struct to setup commands stdout/stdin and execute
        rc = setup_and_execute_command(commands_arr[i], pipe_in, pipes_fd[WRITE_PIPE], envp);
        if (rc < 0) return ERROR;

        // close writing end of pipe, the child will write to it if pipeing out is specified
        rc = close(pipes_fd[WRITE_PIPE]);
        if (rc < 0) return ERROR;

        // store the read end of the previous commands pipe. If another command follows
        // this becomes stdin of the next command
        pipe_in = pipes_fd[READ_PIPE];
    }

    // reset stdin and stdout to default
    rc = reset_stdin_stdout(stdin_copy, stdout_copy);
    if (rc < 0) return ERROR;

    // cleanup executor
    executor_cleanup(envp);

    return SUCCESS;
}
