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

char **make_environ();
void set_environ(char **envp);
bool does_environ_var_exist(char *var_name);
void set_environ_var(char *name, char *value);
void remove_environ_var(char *name);
struct environ_var_t *get_environ_var(char *var_name);

#endif