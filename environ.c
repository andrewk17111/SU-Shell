#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "environ.h"

/**
 * Environment list
 * environment variable struct
 *      name
 *      value
 *      list
 * 
 * char **make_env()
 * void set_environment(char **envp)
 * int does_environ_var_exist(char *var_name)
 * void add_to_environ(evironment variable struct var)
 * evironment variable struct get_environment_variable(char *var_name)
 */