/**
 * @file: sush.c
 * @author: Andrew Kress
 * @author: Michael Permyashkin
 * 
 * @brief: Launches SU Shell prompting for user input
 * 
 * Launches the shell by first initalizing execution environement and any start
 * up commands defined in the .sushrc file. After setup, the shell enters loop 
 * and prompts users for commands to execute.
 * 
 * `exit` command terminates the shell after performing cleanup
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/stat.h>

#include "runner.h"
#include "error.h"
#include "environ.h"
#include "background.h"

// max size of command line input we accept
#define CMD_BUFFER 512 

// typedef for the call back function pointer for signal handler
typedef void (*signal_handler_t)(int);


/**
 * Function to register a callback function to handle given signal
 * 
 * @param callback: function that we register to handle the signal
 * @param sig_id: signal that the callback function will handle
 * 
 * @return: integer T/F if registration was successful
 **/ 
int register_handler(signal_handler_t callback, int sig_id) {
    if (signal(sig_id, callback) == SIG_ERR) {
        return ERROR;
    }
    return SUCCESS;
}


/**
 * If user defined startup command in the .sushrc file and has permission
 * to read and execute commands, the function executes each command
 */ 
void run_startup_commands() {
    if (environ_var_exist("SUSHHOME")) {
        char *sushhome = environ_get_var("SUSHHOME")->value;
        char *filename = malloc(strlen(sushhome) + 9);
        strcpy(filename, sushhome);
        strcat(filename, "/.sushrc");

        // get file permissions
        struct stat sfile;
        stat(filename, &sfile);

        // if user can read and execute
        if ((sfile.st_mode & S_IRUSR) && (sfile.st_mode & S_IXUSR)) {
            FILE *fp = fopen(filename, "r");

            // read and execute each line
            char cmdline[CMD_BUFFER];
            while (fgets(cmdline, CMD_BUFFER, fp)) {
                // if command was read, execute
                if (cmdline != NULL && cmdline[0] != '\n' && cmdline[0] != '\0') {
                    int rc = do_command(cmdline);
                }
            }

            //close file
            fclose(fp);
        }
    }
}


/**
 * Get's the value to be used for the command prompt.
 */
char * get_prompt() {
    char *prompt = ">";

    // get PS1 if exists
    if (environ_var_exist("PS1")) {
        prompt = strdup(environ_get_var("PS1")->value);
    }

    return prompt;
}


/**
 * Launches the shell by first initializing environement, executing any
 * startup command defined in .sushrc file and then enters loop to prompt
 * user for command line input
 */ 
int main(int argc, char *argv[], char *envp[]) {
    int rc;

    // register callback for child death signal, exit on failure
    if (!register_handler(sig_handler, SIGCHLD)) return -1;

    // Environment Setup
    environ_init(envp);

    // Run startup commands
    run_startup_commands();

    // string to hold command line input
    char cmdline[CMD_BUFFER];

    // print prompt
    printf("%s", get_prompt());
    fflush(stdout);

    // prompt user until exit
    while (fgets(cmdline, CMD_BUFFER, stdin) != NULL) {
        // not empty command lines, execute
        if (cmdline[0] != '\n') {
            // execute command line
            rc = do_command(cmdline);

            // exit command success, break loop and exit shell
            if (rc == EXIT_SHELL) break;
        }

        // print prompt again
        printf("%s", get_prompt());
        fflush(stdout);
    }

    // clean up after exit command
    environ_clean_up();

    // clean and free queue
    queue_cleanup();

    // exit shell
    exit(0);
}