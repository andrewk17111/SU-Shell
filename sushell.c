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
#include "internal.h"
#include "internal.h"

#define CMD_BUFFER 512 

int main(int argc, char *argv[]) {
    char buffer[CMD_BUFFER];

    while(1) {
        fgets(buffer, CMD_BUFFER-1, stdin);
        handle_command(buffer);   
    }
    
}