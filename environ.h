#include <stddef.h>
#include "list.h"

#ifndef ENVIRON_H
#define ENVIRON_H

struct environ_var_t {
    char *name;
    char *value;
    struct list_head list;
};

char **make_env();
void set_environment(char **envp);
int does_environ_var_exist(char *var_name);
void add_to_environ(struct environ_var_t var);
struct environ_var_t get_environment_variable(char *var_name);

#endif