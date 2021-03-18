/**
 * File: list.h
 * Purpose: Contains function signatures for linked list operations. Defines
 *      function like macros which we use to initialize a linked list and nodes
 *      of the list.
 * 
 * @author: Michael Permyashkin
 * @author: Andrew Kress
 **/ 

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