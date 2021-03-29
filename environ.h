/**
 * @file: environ.h
 * @author: Andrew Kress
 * 
 * @brief: Header file for the internal environment
 * 
 * Defines the environment variable struct and functions
 * for interacting with the internal environment.
 */ 

#include <stddef.h>
#include <stdbool.h>
#include "list.h"

#ifndef ENVIRON_H
#define ENVIRON_H

struct environ_var_t {
    char *name;
    char *value;
    struct list_head list;
};

/**
 * Returns a string array for the environment variables from the environment list
 * 
 * @return Environment string array
 **/
char **make_environ();
/**
 * Sets up the environment linked list
 * 
 * @param envp - Environment string array like envp from main
 **/
void environ_init(char **envp);
/**
 * Checks if an environment variable exists
 * 
 * @param name - Environment variable name
 * 
 * @return true if variable exists in the environment; false if it doesn't
 **/
bool environ_var_exist(char *name);
/**
 * Add new environment variable if one of the given name
 * doesn't exist or update it if one does exist
 * 
 * @param name - Name of the environment variable
 * @param value - Value of the environment variable
 **/
void environ_set_var(char *name, char *value);
/**
 * Remove an environment variable of the given name
 * if it is in the list
 * 
 * @param name - Name of the environment variable
 **/
void environ_remove_var(char *name);
/**
 * Returns the environment variable that has the
 * given name
 * 
 * @param name - Name of the environment variable
 * 
 * @return The environment variable
 **/
struct environ_var_t *environ_get_var(char *name);
/**
 * Prints the current environment variables and their values
 **/
void environ_print();
/**
 * Cleans up and frees the variables used by environ.c
 */
void environ_clean_up();
#endif