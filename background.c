#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "list.h"
#include "cmdline.h"
#include "internal.h"
#include "executor.h"
#include "environ.h"
#include "background.h"


int job_count = 0;
int job_running = false;
#define LINE_BUFFER 512 


LIST_HEAD(queue_list);


bool is_valid_background_command(struct command_t *command) {
    // stdin cannot be changed
    if (command->pipe_in || command->file_in != REDIRECT_NONE) 
        return false;

    // stdout cannot be changed
    if (command->pipe_out || command->file_in != REDIRECT_NONE) 
        return false;
    
    return true;
}


int set_command_channels(struct command_t *command) {
    // stdin becomes null
    command->file_in = FILE_IN;
    command->infile = "/dev/null";

    // create temp file with pattern
    char template[] = "/tmp/background_cmd_XXXXXXXX";
    int temp_fid = mkstemp(template);
    if (temp_fid < 0) return ERROR;

    // stdout becomes unique temp file at execution
    command->file_out = FILE_OUT_OVERWRITE;
    command->outfile = strdup(template);
    command->fid_out = temp_fid;
    return SUCCESS;
}


void dequeue_and_execute() {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    if (!list_empty(head)) {
        for (curr = head->next; curr != head; curr = curr->next) {
            queue_item = list_entry(curr, struct queue_item_t, list);
            if (!queue_item->is_complete) {
                job_running = true;

                pid_t pid = fork();

                if (pid < 0) return;
                if (pid == 0) {
                    if (is_internal_command(queue_item->command)) {
                        execute_internal_command(queue_item->command);
                    }

                    else {
                        struct command_t **commands_arr = malloc(sizeof(struct command_t) * 1);
                        commands_arr[0] = queue_item->command;

                        execute_external_command(commands_arr, 1);
                        exit(0);
                    }
                } else {
                    // parent stores pid of child
                    queue_item->pid = pid;
                }
            }
        }
    }
    
}

void add_to_queue(struct command_t *command) {
    struct queue_item_t *queue_item = malloc(sizeof(struct queue_item_t));

    queue_item->job_id = job_count++;
    queue_item->is_complete = false;
    queue_item->outfile = command->outfile;
    queue_item->command = command;
    list_add_tail(&queue_item->list, &queue_list);

    if (!job_running) 
        dequeue_and_execute();
}



void print_all_job_status() {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        char *status = (queue_item->is_complete) ? "complete" : "queued";
        printf("%d is %s\n", queue_item->job_id, status);
    }
}



/**
 * Callback function to handle child death. Prints the signal id
 * 
 * @param signal: integer signal identifier
 **/ 
void sigchild_handler(int signal) {
    pid_t pid;

    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);
        pid_t job_pid = queue_item->pid;

        while ((pid = waitpid(job_pid, NULL, WNOHANG)) > 0) {
            job_running = false;
            queue_item->is_complete = true;

            dequeue_and_execute();
        }
    }
}

void print_job_output(int job_id) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        if (job_id == queue_item->job_id) {
            FILE *fp = fopen(queue_item->outfile, "r");

            // read and execute each line
            char line[LINE_BUFFER];
            while (fgets(line, LINE_BUFFER, fp)) {
                printf("%s", line);
            }

            //close file
            fclose(fp);

        }
    }
}


void cancel_job(int job_id) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        if (job_id == queue_item->job_id) {
            list_del(curr);
        }
    }
}