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
    //Environment Setup
    set_environ(envp);

    //Command Processing
    char cmdline[CMD_BUFFER];

    int again = 1;
    while(again) {
        printf("%s> ", get_environ_var("PS1")->value);
        if (fgets(cmdline, CMD_BUFFER-1, stdin) != NULL ) {
            if (strcmp(cmdline, "exit\n") == 0) {
                again = false;
            } else {
                int rc = do_command(cmdline);
            }
        }
    }

    //Clear environ
    
    return 0;
}