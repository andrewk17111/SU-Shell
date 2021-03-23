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
void environ_init(char **envp);
bool environ_var_exist(char *var_name);
void environ_set_var(char *name, char *value);
void environ_remove_var(char *name);
struct environ_var_t *environ_get_var(char *var_name);

#endif