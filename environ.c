/**
 * @file: environ.c
 * @author: Andrew Kress
 * 
 * @brief: Handles the internal environment variables.
 * 
 * Using the different functions from this file,
 * individual environment variables can be created,
 * modified, deleted, and retrieved. The internal
 * environment can also be created by a NULL terminated
 * string array and a NULL terminated string array
 * can be created from the internal environment.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "environ.h"
#include "internal.h"
#include "cmdline.h"

LIST_HEAD(environment);

/**
 * Returns a string array of the name and value for an evironment variable
 * 
 * @param env_var_str - Environment variable string
 * 
 * @return String array for name and value of environ var
 **/
char **split_environ_var(char *env_var_str) {
    int env_var_len = strlen(env_var_str);
    char **env_var_arr = calloc(2, sizeof(char *));
    int pivot;

    for (pivot = 0; pivot < env_var_len; pivot++) {
        if (env_var_str[pivot] == '=')
            break;
    }

    env_var_arr[0] = sub_string(env_var_str, 0, pivot);
    env_var_arr[1] = sub_string(env_var_str, pivot + 1, env_var_len);
    return env_var_arr;
}

/**
 * Returns a string array for the environment variables from the environment list
 * 
 * @return Environment string array
 **/
char **make_environ() {
    char **envp = calloc(list_size(&environment) + 1, sizeof(char *));
    struct list_head *head = &environment;
    struct list_head *curr;
    struct environ_var_t *env_var;
    int i = 0;
    for (curr = head->next; curr != head; curr = curr->next) {
        env_var = list_entry(curr, struct environ_var_t, list);
        char *string = malloc(strlen(env_var->name) + strlen(env_var->value) + 2);
        strcpy(string, env_var->name);
        strcat(string, "=");
        strcat(string, env_var->value);
        envp[i] = string;
        i++;
    }
    envp[i] = NULL;

    return envp;
}

/**
 * Sets up the environment linked list
 * 
 * @param envp - Environment string array like envp from main
 **/
void environ_init(char **envp) {
    // Loop through the given envp values.
    for (int i = 0; envp[i] != NULL; i++) {
        // Create a new environment variable.
        struct environ_var_t *var = malloc(sizeof(struct environ_var_t));
        // Create a string array of the variable name and value.
        char **environ_var_val = split_environ_var(envp[i]);
        // Set the variable name and value.
        var->name = environ_var_val[0];
        var->value = environ_var_val[1];
        // Add the new environment variable to the
        // internal environment list.
        list_add_tail(&var->list, &environment);
        // Free the name and value string array.
        free(environ_var_val);
    }
    // Setup PS1 for the prompt.
    environ_set_var("PS1", ">");
    // Setup SUSHHOME for .sushrc.
    environ_set_var("SUSHHOME", environ_get_var("PWD")->value);
}

/**
 * Checks if an environment variable exists
 * 
 * @param name - Environment variable name
 * 
 * @return true if variable exists in the environment; false if it doesn't
 **/
bool environ_var_exist(char *name) {
    // Loop through the internal environment.
    struct list_head *head = &environment;
    struct list_head *curr;
    struct environ_var_t *env_var;
    for (curr = head->next; curr != head; curr = curr->next) {
        // Get the environment variable for the current list item.
        env_var = list_entry(curr, struct environ_var_t, list);
        // If the environment variable's name matches, return true;
        if (env_var != NULL && strcmp(env_var->name, name) == 0)
            return true;
    }
    // If none of the environment variables match, return false.
    return false;
}

/**
 * Add an environment variable struct to the linked list
 * 
 * @param name - Name of the new environment variable
 * @param value - Value of the new environment variable
 **/
void environ_add_var(char *name, char *value) {
    // Create new environment variable.
    struct environ_var_t *var = malloc(sizeof(struct environ_var_t));
    // Assign environment variable's name and value
    // to the given name and value.
    var->name = strdup(name);
    var->value = strdup(value);
    // Add the environment variable's list item to the list.
    list_add_tail(&var->list, &environment);
}

/**
 * Update an existing environment variable struct in the linked list
 * 
 * @param name - Name of the environment variable
 * @param value - Value of the environment variable
 **/
void environ_update_var(char *name, char *value) {
    // Get the requested environment variable.
    struct environ_var_t *var = environ_get_var(name);
    // Free the old value.
    free(var->value);
    // Set the value to equal the given value.
    var->value = strdup(value);
}

/**
 * Add new environment variable if one of the given name
 * doesn't exist or update it if one does exist
 * 
 * @param name - Name of the environment variable
 * @param value - Value of the environment variable
 **/
void environ_set_var(char *name, char *value) {
    // If the requested environment variable exists,
    // update its value with the given value,
    // otherwise, add it to the list with the given value.
    if (environ_var_exist(name))
        environ_update_var(name, value);
    else
        environ_add_var(name, value);
}

/**
 * Remove an environment variable of the given name
 * if it is in the list
 * 
 * @param name - Name of the environment variable
 **/
void environ_remove_var(char *name) {
    // If the requested environment variable exists,
    // remove it from the list.
    if (environ_var_exist(name)) {
        // Get the given environment variable.
        struct environ_var_t *var = environ_get_var(name);
        // Remove the list item from the internal environment.
        list_del(&var->list);
        // Free the environment variable.
        free(var->name);
        free(var->value);
        free(var);
    }
}

/**
 * Returns the environment variable that has the
 * given name
 * 
 * @param name - Name of the environment variable
 * 
 * @return The environment variable
 **/
struct environ_var_t *environ_get_var(char *name) {
    // If the requested environment variable exists,
    // get the value of the variable.
    if (environ_var_exist(name)) {
        // Loop through the environment variables.
        struct list_head *head = &environment;
        struct list_head *curr;
        struct environ_var_t *env_var;
        for (curr = head->next; curr != head; curr = curr->next) {
            // Get the value of the environment variable for the
            // current list item.
            env_var = list_entry(curr, struct environ_var_t, list);
            // Check if the environment variable's name matches
            // the requested variable and return if it matches.
            if (strcmp(env_var->name, name) == 0)
                return env_var;
        }
    }
    return NULL;
}

/**
 * Prints the current environment variables and their values
 **/
void environ_print() {
    // Loop through the internal environment
    // and print the values.
    char **env = make_environ();
    for (int i = 0; env[i] != NULL; i++) {
        printf("%s\n", env[i]);
        free(env[i]);
    }
    free(env);
}

/**
 * Cleans up and frees the variables used by environ.c
 */
void environ_clean_up() {
    // Loop through the internal environment.
    struct list_head *head = &environment;
    struct list_head *curr = head->next;
    struct environ_var_t *env_var;
    while (curr != head) {
        // Get the environment variable for the list item.
        env_var = list_entry(curr, struct environ_var_t, list);
        // Delete the current list item.
        curr = curr->next;
        list_del(curr->prev);
        // Free the environment variable.
        free(env_var->name);
        free(env_var->value);
        free(env_var);
    }
}