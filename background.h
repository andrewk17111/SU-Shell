
#include <stddef.h>
#include <stdbool.h>
#include <signal.h>

#ifndef BACKGROUND_H
#define BACKGROUND_H



struct queue_item_t {
    pid_t pid;
    int job_id;
    char *outfile;
    bool is_complete;

    struct command_t *command;
    struct list_head list;
};


int is_background_command(char *cmdline);

bool is_valid_background_command(struct command_t *command);

void background_command_handler(char *cmdline);

int set_command_channels(struct command_t *command);

void add_to_queue(struct command_t *command);

void sigchild_handler(int signal);

void print_all_job_status();

void print_job_output();

int remove_from_queue();

#endif
