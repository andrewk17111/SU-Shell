#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "list.h"
#include "background.h"


#define WRITE 1
#define READ 0
#define CMD_BUFFER 512 

int is_forked = false;
int pipes_fd[2];
LIST_HEAD(queue);

void wait_commands(int pipe_read) {
    // pipes read end becomes stdin
    close(STDIN_FILENO);
    dup(pipe_read);
    close(STDOUT_FILENO);

    char cmdline[CMD_BUFFER];
    while (true) {
        fgets(cmdline, CMD_BUFFER - 1, stdin);
        // printf("got commmand: (%s)\n", cmdline);
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



/**
 * Returns a string that starts at the given index at the length given
 * 
 * @param str - The source string you want to get a part of
 * @param start - The starting index of the substring
 * @param length - The length of the substring
 * @return extracted substring
 */
char * sub_string_2(char* str, int start, int length) {
    char* output = calloc(length + 1, sizeof(char));
    for (int i = start; i < (start + length) && i < strlen(str); i++) {
        output[i - start] = str[i];
    }
    return output;
}

int is_background_command(char *cmdline) {
    char *cmd;
    int len = strlen(cmdline);

    for (int i=0; i<len; i++) {
        if (cmdline[i] == ' ' || cmdline[i] == '\n') {
            cmd = sub_string_2(cmdline, 0, i);
            return (strcmp(cmd, "queue") == 0);
        }
    }

    return false;
}





