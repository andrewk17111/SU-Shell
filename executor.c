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



/**
 * Child process runs the given executable with the provided args
 * 
 * @param executable: the same of the executable we want to run
 * @param argv: arguments to use with the executable
 **/ 
void do_child(const char *executable, char *const argv[], char *const envp[]) {
    int status = execve(executable, argv, envp);
    // if exec returns, something went wrong
    printf("exec returned! errno is [%d]\n",errno);
    exit(-1);
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


/**
 * Function that takes an executable we want to run with the given arguments.
 * Forks the process and the child executes the command. The parent waits for 
 * the child to finish and prints the return status. 
 * 
 * And error message is printed if exec is unable to run the command successfully
 * 
 * @param executable: path of the executable we want to run
 * @param argv: The argument list passed to exec
 **/ 
void run_cmd(const char *executable, char *const argv[], char *const envp[]) {
    pid_t pid;
    pid = fork();
    
    // check fork() return to ensure it is a valid pid, otherwise an error occured
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return;
    } else if (pid == 0) {
        do_child(executable, argv, envp);
    } else {
        do_parent(pid);
    }
}




int execute_external_command(struct command_t *commands_arr[], int num_commands) {

    char **envp = make_environ();

    if (num_commands == 1) {
        char *executable = commands_arr[0]->cmd_name;
        char **argv = commands_arr[0]->tokens;

        run_cmd(executable, argv, envp);
    }

    return 0;
}
