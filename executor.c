/**
 * @file: executor.c
 * @author: Michael Permyashkin
 * 
 * @brief: Executes an array of commands
 * 
 * Driver function is called by runner.c which provides an array of non-internal commands
 * to execute. This execution unit handles all redirection of commands as well as piping
 * between contiguous commands.
 */ 
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "cmdline.h"
#include "error.h"
#include "executor.h"
#include "environ.h"

#include <errno.h>
#include <fcntl.h>


/**
 * Resets stdin and stdout after command(s) execution to ensure
 * both are reset to default.
 * 
 * @param stdin_copy: default stdin 
 * @param stdout_copy: default stdout 
 * 
 * @return status of reset
 */ 
int reset_stdin_stdout(int stdin_copy, int stdout_copy) {
    int rc;

    rc = dup2(stdin_copy, STDIN_FILENO);
    if (rc < 0) return RETURN_ERROR;
    
    rc = dup2(stdout_copy, STDOUT_FILENO);
    if (rc < 0) return RETURN_ERROR;
    
    rc = close(stdin_copy);
    if (rc < 0) return RETURN_ERROR;

    rc = close(stdout_copy);
    if (rc < 0) return RETURN_ERROR;

    return RETURN_SUCCESS;
}


/**
 * Creates file for redirection out. Takes the name of the file and the type
 * of redirection to be done (overwrite or append). If overwriting, the file is
 * created if it does not exist or truncates the existing file. If appending, the file 
 * is created if it does not exist, or opens it for appending.
 * 
 * @param fname: name of the file
 * @param redir_type: type of redirection out to be done 
 * 
 * @return file id if file was opened successfully
 */ 
int create_out_file(char *fname, enum redirect_type_e redir_type) {
    int fid;

    // if create/overwrite
    if (redir_type == FILE_OUT_OVERWRITE)
        fid = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);

    // if append
    if (redir_type == FILE_OUT_APPEND)
        fid = open(fname, O_RDWR | O_CREAT | O_APPEND, 0777);

    if (fid < 0) return RETURN_ERROR;
    return fid;
}


/**
 * Opens file for redirection in. If the filename exists, the file is opened for
 * read and returns the file id. If the file does not 
 * 
 * @param fname: name of the file
 * @param redir_type: type of redirection out to be done 
 * 
 * @return file id if file was opened successfully
 */ 
int open_in_file(char *fname) {
    int fid = open(fname, O_RDONLY, 0777);

    if (fid < 0) return RETURN_ERROR;
    return fid;
}


/**
 * Creates and stores appropriate file id's in the command struct
 * for any redirection that is defined.
 * 
 * @param command: command to check for any redirection
 * 
 * @return status of redirection file setup 
 */ 
int setup_command_redirection(struct command_t *command) {
    int fid;

    // redirection out
    if (command->file_out) {
        fid = create_out_file(command->outfile, command->file_out);

        if (fid < 0) return RETURN_ERROR;
        command->fid_out = fid; 
    }

    // redirection in
    if (command->file_in) {
        fid = open_in_file(command->infile);

        if (fid < 0) return RETURN_ERROR;
        command->fid_in = fid;
    }

    return RETURN_SUCCESS;
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
 * 
 * @return will only return if exec fails
 **/ 
int do_child(struct command_t *command, int pipe_in, int pipe_out, char *const envp[]) {
    int rc;

    // if redirection out
    if (command->file_out) {
        rc = close(STDOUT_FILENO);
        if (rc < 0) return RETURN_ERROR;
        
        rc = dup(command->fid_out);
        if (rc < 0) return RETURN_ERROR;
    }

    // if redirection in
    if (command->file_in) {
        rc = close(STDIN_FILENO);
        if (rc < 0) return RETURN_ERROR;
       
        rc = dup(command->fid_in);
        if (rc < 0) return RETURN_ERROR;
    } 

    // if piping in
    if (command->pipe_in) {
        // you are not the last command -> stdin from pipes
        rc = close(STDIN_FILENO);
        if (rc < 0) return RETURN_ERROR;

        rc = dup2(pipe_in, STDIN_FILENO);
        if (rc < 0) return RETURN_ERROR;
    }

    // if piping out
    if (command->pipe_out) {
        // you are not the last command -> stdout to pipes
        rc = close(STDOUT_FILENO);
        if (rc < 0) return RETURN_ERROR;

        rc = dup2(pipe_out, STDOUT_FILENO);
        if (rc < 0) return RETURN_ERROR;
    }

    // execute command
    execvpe(command->cmd_name, command->tokens, envp);

    // if exec returns, something went wrong
    printf("exec returned! errno is [%d]\n",errno);
    return RETURN_ERROR;
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
 * 
 * @return status of command execution
 */ 
int fork_and_exec(struct command_t *command, int pipe_in, int pipe_out, char *const envp[]) {
    pid_t pid = fork();
    
    // check fork() return to ensure it is a valid pid, otherwise an error occured
    if (pid < 0) {
        return RETURN_ERROR;
    } else if (pid == 0) {
        do_child(command, pipe_in, pipe_out, envp);
    } else {
        do_parent(pid);
    }

    return RETURN_SUCCESS;
}


/**
 * Each command uses this function to first setup all redirection files, create pipes, forks and
 * executes the command given.
 * 
 * @param command: command struct holding information about commands execution config
 * @param pipe_in: read side of the pipe used if given command proceeds another
 * @param pipe_out: write side of the pipe used if given command preceeds another
 * @param envp: environement for command execution
 * 
 * @return status of command execution
 */ 
int setup_and_execute_command(struct command_t *command, int pipes_fd[2], int pipe_in, char *const envp[]) {
    int rc;

    // creates/opens any files to be used for command redirection
    rc = setup_command_redirection(command);
    if (rc < 0) return RETURN_ERROR;

    rc = pipe(pipes_fd);
    if (rc < 0) return RETURN_ERROR;

    rc = fork_and_exec(command, pipe_in, pipes_fd[1], envp);
    if (rc < 0) return RETURN_ERROR;

    return RETURN_SUCCESS;
}


/**
 * Driver function which executes an array of commands using the information stored in each command stuct to determine
 * the behavior of each commands execution.
 * 
 * @param commands_arr: array of command structs to execute
 * @param num_commands: the number of commands to execute
 * 
 * @return status of all setup tasks and all commands execution
 */ 
int execute_external_command(struct command_t *commands_arr[], int num_commands) {

    // store default stdin and stdout to reset after all execution
    int stdin_copy = dup(STDIN_FILENO);
    int stdout_copy = dup(STDOUT_FILENO);

    char **envp = make_environ();
    
    int i=0; // starting index of command in array of commands
    int rc;
    pid_t pid;

    // create pipes
    int pipes_fd[ 2 ];
    int pipe_in = 0;

    // fork and execute each command except last (or first if only 1 command is given)
    for (i=0; i<num_commands-1; ++i) {
        // use struct to setup execution behavior and execute command
        rc = setup_and_execute_command(commands_arr[i], pipes_fd, pipe_in, envp);
        if (rc < 0) return RETURN_ERROR;

        // close writing end of pipe, the child will write to it's copy
        rc = close(pipes_fd[1]);
        if (rc < 0) return RETURN_ERROR;

        // becomes the read end of the next command
        pipe_in = pipes_fd[0];
    }


    // if pipe_in is no longer 0, previous command is piping into the last command to execute
    if (pipe_in != 0) {
        rc = dup2(pipe_in, STDIN_FILENO);
        if (rc < 0) return RETURN_ERROR;
    }

    // setup and execute remaining (or single) command
    rc = setup_and_execute_command(commands_arr[i], pipes_fd, pipe_in, envp);
    if (rc < 0) return RETURN_ERROR;

    // reset stdin and stdout to default
    rc = reset_stdin_stdout(stdin_copy, stdout_copy);
    if (rc < 0) return RETURN_ERROR;

    return RETURN_SUCCESS;
}
