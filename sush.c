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
#include "cmdline.h"
#include "error.h"
#include "environ.h"

#define CMD_BUFFER 512 

int main(int argc, char *argv[], char *envp[]) {
    int rc;

    //Environment Setup
    environ_init(envp);

    //Command Processing
    char cmdline[CMD_BUFFER];
    while(1) {
        printf("%s ", environ_get_var("PS1")->value);
        fflush(stdout);
        if (fgets(cmdline, CMD_BUFFER-1, stdin) != NULL ) {
            rc = do_command(cmdline);
        }
    }

    //Clear environ
    environ_clean_up();

    return 0;
}