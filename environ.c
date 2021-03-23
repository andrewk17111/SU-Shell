#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "environ.h"

LIST_HEAD(environment);

char *environ_sub_string(char* str, int start, int length) {
    char* output = calloc(length + 1, sizeof(char));
    for (int i = start; i < (start + length) && i < strlen(str); i++) {
        output[i - start] = str[i];
    }
    return output;
}

char **split_environ_var(char *env_var_str) {
    int env_var_len = strlen(env_var_str);
    char **env_var_arr = calloc(2, sizeof(char *));
    int pivot;

    for (pivot = 0; pivot < env_var_len; pivot++) {
        if (env_var_str[pivot] == '=')
            break;
    }

    env_var_arr[0] = environ_sub_string(env_var_str, 0, pivot);
    env_var_arr[1] = environ_sub_string(env_var_str, pivot + 1, env_var_len);
    return env_var_arr;
}

char **make_environ() {
    char **envp = calloc(list_size(&environment) + 1, sizeof(char *));
    struct list_head *head = &environment;
    struct list_head *curr;
    struct environ_var_t *env_var;
    int i = 0;
    for (curr = head->next; curr != head; curr = curr->next) {
        env_var = list_entry(curr, struct environ_var_t, list);
        envp[i] = strdup(strcat(strcat(env_var->name, "="), env_var->value));
        i++;
    }

    envp[i] = NULL;

    return envp;
}

void set_environ(char **envp) {
    for (int i = 0; envp[i] != NULL; i++) {
        struct environ_var_t *var = malloc(sizeof(struct environ_var_t));
        char ** environ_var_val = split_environ_var(envp[i]);
        var->name = environ_var_val[0];
        var->value = environ_var_val[1];
        list_add_tail(&var->list, &environment);
    }
    set_environ_var("PS1", "~");
}

bool does_environ_var_exist(char *var_name) {
    struct list_head *head = &environment;
    struct list_head *curr;
    struct environ_var_t *env_var;
    for (curr = head->next; curr != head; curr = curr->next) {
        env_var = list_entry(curr, struct environ_var_t, list);
        if (strcmp(env_var->name, var_name) == 0)
            return true;
    }
    return false;
}

void add_environ_var(char *name, char *value) {
    struct environ_var_t var = { .name = name, .value = value };
    list_add_tail(&var.list, &environment);
}

void update_environ_var(char *name, char *value) {
    struct environ_var_t *var = get_environ_var(name);
    var->value = value;
}

void set_environ_var(char *name, char *value) {
    if (does_environ_var_exist(name))
        update_environ_var(name, value);
    else
        add_environ_var(name, value);
}

struct environ_var_t *get_environ_var(char *name) {
    if (does_environ_var_exist(name)) {
        struct list_head *head = &environment;
        struct list_head *curr;
        struct environ_var_t *env_var;
        for (curr = head->next; curr != head; curr = curr->next) {
            env_var = list_entry(curr, struct environ_var_t, list);
            if (strcmp(env_var->name, name) == 0)
                return env_var;
        }
    }
    return NULL;
}

void remove_environ_var(char *name) {
    struct environ_var_t *var = get_environ_var(name);
    if (var != NULL)
        list_del(&var->list);
}

//environ_cleaup