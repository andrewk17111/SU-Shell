#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cmdline.h"
#include "internal.h"
#include "executor.h"
#include "environ.h"
#include "background.h"


#define WRITE 1
#define READ 0
#define CMD_BUFFER 512 

int is_forked = false;
int pipes_fd[2];
LIST_HEAD(queue);


bool is_valid_background_command(struct command_t *command) {
    // stdin cannot be changed
    if (command->pipe_in || command->file_in != REDIRECT_NONE) 
        return false;

    // stdout cannot be changed
    if (command->pipe_out || command->file_in != REDIRECT_NONE) 
        return false;
    
    return true;
}


int set_command_channels(struct command_t *command) {
    // stdin becomes null
    command->file_in = FILE_IN;
    command->infile = "/dev/null";

    // create temp file with pattern
    char template[] = "/tmp/background_cmd_XXXXXXXX";
    int temp_fid = mkstemp(template);
    if (temp_fid < 0) return ERROR;

    // stdout becomes unique temp file at execution
    command->file_out = FILE_OUT_OVERWRITE;
    command->outfile = template;
    command->fid_out =temp_fid;
    return SUCCESS;
}


int parse_execute_background_command(char *background_cmd) {
    // parser returns array of commands, initialize array to hold 1
    struct command_t **command_arr = malloc(sizeof(struct command_t) * 1);
    
    int rc = parse_command(command_arr, 1, background_cmd);
    if (rc < 0) return ERROR;

    if (is_valid_background_command(command_arr[0])) {

        // set commands stdin to null and stdout to a temp file
        rc = set_command_channels(command_arr[0]);
        if (rc < 0) return ERROR;

        if (is_internal_command(command_arr[0])) {
            execute_internal_command(command_arr[0]);
        } else {
            rc = execute_external_command(command_arr, 1);
        }

        // remove temp file
        remove(command_arr[0]->outfile);
    }
}


char * remove_first_token(char *cmdline) {
    int i = 0;
    int len = strlen(cmdline);

    while(cmdline[i] != ' ' && i < len) {
        i++;
    }
    return sub_string(cmdline, i, len-1);
}


void wait_commands(int pipe_read) {
    // pipes read end becomes stdin
    close(STDIN_FILENO);
    dup(pipe_read);

    char cmdline[CMD_BUFFER];
    while (true) {
        fgets(cmdline, CMD_BUFFER, stdin);

        // remove first token which is the internal command specifying cmdline is background
        char *background_cmd = remove_first_token(cmdline);

        int rc = parse_execute_background_command(background_cmd);
    }
}


void send_commands(int pipe_write, char *cmdline) {
    write(pipe_write, cmdline, strlen(cmdline));
}


void background_command_handler(char *cmdline) {

    if (!is_forked) {
        // Create pipe or return if error occured
        int rc = pipe(pipes_fd); 
        if (rc < 0) return;

        pid_t pid;
        pid = fork();

        // check fork() return to ensure it is a valid pid, otherwise an error occured
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            return;
        } else if (pid == 0) {
            wait_commands(pipes_fd[READ]);
        } else {
            send_commands(pipes_fd[WRITE], cmdline);
        }
        is_forked = true;
    }
    // forked already
    else {
        send_commands(pipes_fd[WRITE], cmdline);
    }
}





int is_background_command(char *cmdline) {
    char *cmd;
    int len = strlen(cmdline);

    for (int i=0; i<len; i++) {
        if (cmdline[i] == ' ' || cmdline[i] == '\n') {
            cmd = sub_string(cmdline, 0, i);
            return (strcmp(cmd, "queue") == 0);
        }
    }

    return false;
}
