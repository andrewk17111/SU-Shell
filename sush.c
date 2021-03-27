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

void run_startup_commands() {
    //Setup filename
    char *sushhome = environ_get_var("SUSHHOME")->value;
    char *filename = malloc(strlen(sushhome) + 9);
    strcpy(filename, sushhome);
    strcat(filename, "/.sushrc");

    struct stat sfile;
    stat(filename, &sfile);

    if ((sfile.st_mode & S_IRUSR) && (sfile.st_mode & S_IXUSR)) {
        FILE *fp = fopen(filename, "r");

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

        fclose(fp);
    }
}

int main(int argc, char *argv[], char *envp[]) {
    int rc;

    //Environment Setup
    environ_init(envp);

    //Run startup commands
    run_startup_commands();

    //Command Processing
    char cmdline[CMD_BUFFER];
    while(1) {
        printf("%s ", environ_get_var("PS1")->value);
        fflush(stdout);
        if (fgets(cmdline, CMD_BUFFER-1, stdin) != NULL ) {
            if (cmdline[0] != '\n') {
                rc = do_command(cmdline);
                if (rc == 2) {
                    break;
                }
            }
        }
    }

    //Clear environ
    environ_clean_up();

    return 0;
}