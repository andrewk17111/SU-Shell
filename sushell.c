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

#define CMD_BUFFER 512 

int main(int argc, char *argv[]) {
    char cmdline[CMD_BUFFER];

    while(1) {

        if ( fgets(cmdline, CMD_BUFFER-1, stdin) != NULL ) {
            int rc = handle_command(cmdline, strlen(cmdline));   
        }

    }
    
}