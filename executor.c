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


int create_out_file(char *fname, enum redirect_type_e redir_type) {
    // if create/overwrite
    if (redir_type == FILE_OUT_OVERWRITE)
        return open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);

    // if append
    if (redir_type == FILE_OUT_APPEND)
        return open(fname, O_RDWR | O_CREAT | O_APPEND, 0777);

    return -1;
}

int create_in_file(char *fname) {
    return open(fname, O_RDONLY, 0777);
}


void handle_cmd_redirection(struct command_t *command) {
    // redirection out
    if (command->file_out) {
        int fid = create_out_file(command->outfile, command->file_out);

        if (fid < 0) 
            RETURN_ERROR;
        else    
            command->fid_out = fid;
    }

    // redirection in
    if (command->file_in) {
        int fid = create_in_file(command->infile);

        if (fid < 0) 
            RETURN_ERROR;
        else    
            command->fid_in = fid;
    }
}


/**
 * Child process runs the given executable with the provided args
 * 
 * @param executable: the same of the executable we want to run
 * @param argv: arguments to use with the executable
 **/ 
int do_child(struct command_t *commands_arr[], int cmd_position,  int pipe_in, int pipe_out, char *const envp[]) {
    int rc;
    struct command_t *command = commands_arr[cmd_position];

    // if redirection out
    if (command->file_out) {
        close(STDOUT_FILENO);
        dup(command->fid_out);
    }

    // if redirection in
    else if (command->file_in) {
        close(STDIN_FILENO);
        dup(command->fid_in);
    } 

    // if piping in
    else if (command->pipe_in) {
        // you are not the last command -> stdin from pipes
        close(STDIN_FILENO);
        rc = dup2(pipe_in, STDIN_FILENO);
        if (rc < 0)
            return RETURN_ERROR;
    }

    // if piping out
    else if (command->pipe_out) {
        // you are not the last command -> stdout to pipes
        close(STDOUT_FILENO);
        rc = dup2(pipe_out, STDOUT_FILENO);
        if (rc < 0)
            return RETURN_ERROR;
    }

    // now we can close all pipes since they would have been duped at this point
    if (pipe_in != 0) {
        dup2(pipe_in, 0);
        close(pipe_in);
    }

    if (pipe_out != 1) {
        dup2(pipe_out, 1);
        close(pipe_out);
    }

    // execute 
    int status = execvpe(command->cmd_name, command->tokens, envp);
    // if exec returns, something went wrong
    printf("exec returned! errno is [%d]\n",errno);
    return RETURN_ERROR;

}


/**
 * parent process just waits for the child to finish
 * 
 * @param pid: id of the child process
 **/ 
void do_parent(int pid) {
    int status;
    waitpid(pid, &status, 0);
}



int execute_external_command(struct command_t *commands_arr[], int num_commands) {

    char **envp = make_environ();
    int rc;
    pid_t pid;
    int num_pipes = num_commands - 1;
    int pipes_fd[ 2 ];

    
    // if more than 1 command we need pipes
    if (num_commands != 1) {
        int i;
        int pipe_in = 0;
        for (i=0; i<num_commands-1; ++i) {
            handle_cmd_redirection(commands_arr[0]);

            rc = pipe(pipes_fd);
            if (rc < 0) RETURN_ERROR;

            pid = fork();
            
            // check fork() return to ensure it is a valid pid, otherwise an error occured
            if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return RETURN_ERROR;
            } else if (pid == 0) {
                do_child(commands_arr, i, pipe_in, pipes_fd[1], envp);
            }

            // child will write to this side, we can close
            close(pipes_fd[1]);

            // save the read end of the pipe, this will become the stdin of the next command
            pipe_in = pipes_fd[0];

        }

        // stdout becomes the read end of the last pipe
        if (pipe_in != 0) {
            dup2(pipe_in, STDIN_FILENO);
        }

        int status = execvpe(commands_arr[i]->cmd_name, commands_arr[i]->tokens, envp);

    } 



    // only 1 command to execute
    else {

        // handle redirection for first command
        handle_cmd_redirection(commands_arr[0]);

        pid = fork();
        
        // check fork() return to ensure it is a valid pid, otherwise an error occured
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            return RETURN_ERROR;
        } else if (pid == 0) {
            do_child(commands_arr, 0, -1, -1, envp);
        } else {
            do_parent(pid);
        }

    }
    

    return 0;
}
