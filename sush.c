/**
 * @file: sushell.c
 * @author: Andrew Kress
 * @author: Michael Permyashkin
 * 
 * @brief: Launches SU Shell prompting for user input
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "cmdline.h"
#include "error.h"
#include "environ.h"

#define CMD_BUFFER 512 

/**
 * If user defined startup command in the .sushrc file and has permission
 * to read and execute commands, the function executes each command
 */ 
void run_startup_commands() {
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
        while (fgets(cmdline, CMD_BUFFER - 2, fp)) {
            //printf("STARTUP: %s\n", cmdline);
            strcat(cmdline, "\n");
            if (cmdline != NULL && cmdline[0] != '\n' && cmdline[0] != '\0') {
                int rc = do_command(cmdline);
                if (rc == 2) {
                    break;
                }
            }
        }

        //close file
        fclose(fp);
    }
}

/**
 * Launches the shell by first initializing environement, executing any
 * startup command defined in .sushrc file and then enters loop to prompt
 * user for command line input
 */ 
int main(int argc, char *argv[], char *envp[]) {
    int rc;

    // Environment Setup
    environ_init(envp);

    // Run startup commands
    run_startup_commands();

    // Command Processing
    char cmdline[CMD_BUFFER];
    while(1) {
        printf("%s ", environ_get_var("PS1")->value);
        fflush(stdout);

        if (fgets(cmdline, CMD_BUFFER-1, stdin) != NULL ) {

            // if not empty command, execute
            if (cmdline[0] != '\n') {
                rc = do_command(cmdline);
                if (rc == 2) {
                    break;
                }
            }
        }
    }

    return 0;
}