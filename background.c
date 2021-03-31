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


// counter to assign job ids
int job_count = 0;
// flag to signal that a job is running in background
int job_running = false;
// buffer to scan lines from output file
#define LINE_BUFFER 512 

// linked list to hold the queue of commands
LIST_HEAD(queue_list);


/**
 * Checks that no redirection or piping is present
 * 
 * @param command: command to validate
 */ 
bool is_valid_background_command(struct command_t *command) {
    // stdin cannot be changed
    if (command->pipe_in || command->file_in != REDIRECT_NONE) 
        return false;

    // stdout cannot be changed
    if (command->pipe_out || command->file_in != REDIRECT_NONE) 
        return false;
    
    return true;
}


/**
 * Sets background command's stdin and stdout. Stdin is closed
 * and stdout always goes to a unique temp file. This configuration
 * is stored in the command struct prior to adding it to the queue
 * 
 * @param command: command to set stdin and stdout fields
 * @return status of setup success
 */ 
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


/**
 * Item is dequeued and executed by child process. Parent updates
 * the pid of the queue_item and returns. Signal handler handles
 * the childs death.
 */ 
void fork_and_execute_background(struct queue_item_t *queue_item) {
    pid_t pid = fork();

    if (pid < 0) return;
    // if child, execute command
    if (pid == 0) {
        // if internal, call internal command executor
        if (is_internal_command(queue_item->command)) {
            execute_internal_command(queue_item->command);
        } 
        // if non-internal, call external command executor
        else {
            struct command_t **commands_arr = malloc(sizeof(struct command_t) * 1);
            commands_arr[0] = queue_item->command;
            execute_external_command(commands_arr, 1);
            exit(0);
        }
    // if parent, set pid of queue item being executed
    } else {
        queue_item->pid = pid;
    }
}


/**
 * Dequeues and executes the next job that in the queue that has not been
 * completed. Sets the job_running flag to true so the shell knows that a 
 * command is being executed in the background.
 */ 
void dequeue_and_execute() {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    if (!list_empty(head)) {
        for (curr = head->next; curr != head; curr = curr->next) {
            queue_item = list_entry(curr, struct queue_item_t, list);

            // find first job that has not been completed
            if (!queue_item->is_complete) {
                job_running = true;
                fork_and_execute_background(queue_item);
                return;
            }
        }
    }
}


/**
 * Initialized queue_item struct and adds the item to the back of the 
 * command queue
 * 
 * @param command: command to add to queue
 */ 
void add_to_queue(struct command_t *command) {
    struct queue_item_t *queue_item = malloc(sizeof(struct queue_item_t));

    // initialize queue item and assign next job id
    queue_item->job_id = job_count++;
    queue_item->is_complete = false;
    queue_item->outfile = command->outfile;
    queue_item->command = command;

    // add item to back of queue
    list_add_tail(&queue_item->list, &queue_list);

    // if nothing is running in background, all commands are done or this is the first so execute
    if (!job_running) 
        dequeue_and_execute();
}


/**
 * Prints the status of all commands to the console except the command that is
 * currently being executed in the background
 */ 
void print_all_job_status() {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        // command is complete
        if (queue_item->is_complete)
            printf("%d is complete\n", queue_item->job_id);
        // command is queued and not running so no pid has been assigned
        else if (queue_item->pid == 0)
            printf("%d - is queued\n", queue_item->job_id);
    }
}


/**
 * Registered to handle all SIGCHLD signals. Handles the death of the child 
 * by checking if the child was a command that was just run in the background. 
 * If it is, the completed command is updated as complete and the next command 
 * is dequeued.
 * 
 * @param signal: signal will be SIGCHLD
 */ 
void sigchild_handler(int signal) {
    pid_t pid;
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    bool run_next = false; // set to true if the child was a background command

    // scan queue and check if pid matches
    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);
        pid_t job_pid = queue_item->pid;
        
        // id pid matches, this was the command that just completed
        if ((pid = waitpid(job_pid, NULL, WNOHANG)) > 0) {
            job_running = false;
            queue_item->is_complete = true;
            run_next = true;
            break;
        }
    }

    // child was the previous background command, run the next in queue
    if (run_next)
        dequeue_and_execute();
}



/**
 * When command is canceled or output is viewed, the command is removed from the queue
 * and the output file is deleted. Handles removing the temporary output file and freeing
 * memory allocated to queue item.
 */ 
void delete_file_and_remove_command(struct queue_item_t *queue_item) {
    // delete file
    remove(queue_item->outfile);
    // remove from queue
    list_del(&queue_item->list);

    free(queue_item);
}


/**
 * Prints the output of the command with the specified job id if the command is complete. 
 * The output of the command is stored in a temporary file and the contents is printed to 
 * the console. Once commands output is viewed, the command is removed from queue and file
 * deleted.
 * 
 * @param job_id: id of job to print output file and remove from queue
 */ 
void print_output_and_remove(int job_id) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        if (job_id == queue_item->job_id) {
            FILE *fp = fopen(queue_item->outfile, "r");

            // read and print each line of file
            char line[LINE_BUFFER];
            while (fgets(line, LINE_BUFFER, fp)) {
                printf("%s", line);
            }
            // close file
            fclose(fp);

            // delete file and remove command from queue 
            delete_file_and_remove_command(queue_item);
            break;
        }
    }
}


/**
 * Attempts to cancel command if not yet complete. If not yet complete the command is removed
 * from the queue and deleted. Otherwise the function returns an error signaling that the command
 * has already been completed and cannot be executed.
 * 
 * @param job_id: id of job to attempt cancel
 * @return: status of cancel attempt
 */ 
int attempt_cancel_command(int job_id) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);
        
        if (job_id == queue_item->job_id) {
            // unable to cancel if command is complete or currently running
            if (queue_item->is_complete || queue_item->pid != 0) {
                return ERROR;
            } 
            // able to cancel, remove from queue
            else {
                delete_file_and_remove_command(queue_item);

                return SUCCESS; 
            }
        }
    }
}