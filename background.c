/**
 * @file: background.c
 * @author: Michael Permyashkin
 * 
 * @brief: functions for shell queue feature for background processesing of commands
 * 
 * The shell supports the ability to queue commands to run in the background one at a time.
 * This file defines the functions to support the internal commands to untilize this feature.
 * The queue is a linked list which commands are enqueued and dequeued for execution. All commands
 * output is redirected to a temporary file which can only be viewed once before the command is
 * removed from the queue and the file is deleted.
 */ 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "list.h"
#include "runner.h"
#include "internal.h"
#include "executor.h"
#include "error.h"
#include "environ.h"
#include "background.h"


// buffer to scan lines from output file
#define LINE_BUFFER 512 

// counter to assign job ids
static int job_count = 0;
// flag to signal that a job is running in background
int job_running = false;

// linked list to hold the queue of commands
static LIST_HEAD(queue_list);


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
    queue_item->outfile = strdup(command->outfile);
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
        if (queue_item->is_complete) {
            LOG_MSG(MSG_STATUS_COMPLETE, queue_item->job_id);
        }
        // command is queued and not running so no pid has been assigned
        else if (queue_item->pid == 0) {
            LOG_MSG(MSG_STATUS_QUEUED, queue_item->job_id);
        }
        // command is runnning and not complete
        else {
            LOG_MSG(MSG_STATUS_RUNNING, queue_item->job_id, queue_item->job_id);
        }
    }
}


/**
 * When command is canceled or output is viewed, the command is removed from the queue
 * and the output file is deleted. Handles removing the temporary output file and freeing
 * memory allocated to queue item.
 */ 
void delete_file_and_remove_command(struct queue_item_t *queue_item) {
    // free all tokens and token array
    int num_toks = queue_item->command->num_tokens;
    for (int j=0;j<num_toks; j++ ) {
        free(queue_item->command->tokens[j]);
    }
    free(queue_item->command->tokens);

    // free filename output file template name
    free(queue_item->command->outfile);
    
    // free command struct
    free(queue_item->command);

    // remove from queue
    list_del(&queue_item->list);

    // delete temp file 
    remove(queue_item->outfile);
    free(queue_item->outfile);

    // free memory storing output file name
    free(queue_item);
}


/**
 * Call back function to handle all signals registered in main shell function. 
 * Our shell registers this function to handle SIGCHLD in order to determine if jobs
 * from the queue have been completed and the next should be run. 
 * 
 * This function is also used to confirm if a job in the background was cancelled successfully.
 * 
 * @param signal: signal id
 */ 
void sig_handler(int signal) {
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

            // job completed normally
            if (signal == SIGCHLD) {
                queue_item->is_complete = true;
            }
            // job was sent a kill signal to cancel
            if (signal == SIGKILL) {
                delete_file_and_remove_command(queue_item);
                LOG_MSG(MSG_CANCEL_OK, queue_item->job_id);
            }

            run_next = true;
        }
    }

    // child was the previous background command, run the next in queue
    if (run_next)
        dequeue_and_execute();
}


/**
 * Checks if a given command is currently in the queue. This is used when doing any
 * cleanup so we avoid double freeing a pointer.
 * 
 * @param command: command to search for in queue
 */ 
bool is_command_in_queue(struct command_t *command) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        // queue item points to same address
        if (queue_item->command == command) {
            return true;
        }
    }

    return false;
}


/**
 * Prints contents of an output file given the file name/path
 * 
 * @param fname: name/path to file
 */ 
void print_output_file(char *fname) {
    FILE *fp = fopen(fname, "r");

    // read and print each line of file
    char line[LINE_BUFFER];
    while (fgets(line, LINE_BUFFER, fp)) {
        printf("%s", line);
    }
    // close file
    fclose(fp);
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

            // job is still running
            if (queue_item->pid != 0 && !queue_item->is_complete) {
                // Print error if there is one or more args
                LOG_ERROR(ERROR_OUTPUT_RUNNING, queue_item->job_id);
            }
            // job still in queue and not complete
            else if (queue_item->pid == 0 && !queue_item->is_complete) {
                // Print error if there is one or more args
                LOG_ERROR(ERROR_OUTPUT_QUEUED, queue_item->job_id);
            }
            // job is complete, display output and remove file and queue item
            else {
                // print contents of file
                print_output_file(queue_item->outfile);
                // delete file and remove command from queue 
                delete_file_and_remove_command(queue_item);
            }
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
 */ 
void attempt_cancel_command(int job_id) {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);
        
        if (job_id == queue_item->job_id) {
            // unable to cancel, job is done
            if (queue_item->is_complete && queue_item->pid != 0) {
                LOG_ERROR(ERROR_CANCEL_DONE, job_id, job_id);
            } 
            // attempt kill, job is running
            else if (!queue_item->is_complete && queue_item->pid != 0) {
                LOG_ERROR(MSG_CANCEL_KILL, job_id, queue_item->pid);
                kill(queue_item->pid, SIGKILL);
            } 
            // able to cancel, remove from queue
            else {
                delete_file_and_remove_command(queue_item);
            }
        }
    }
}


/**
 * Removes all items from the queue and deletes output files for each and finishes
 * by freeing all memory allocated to queue items and commands. This function executes
 * when the shell recieves an exit status.
 */ 
void queue_cleanup() {
    struct list_head *head = &queue_list;
    struct list_head *curr;
    struct queue_item_t *queue_item;

    for (curr = head->next; curr != head; curr = curr->next) {
        queue_item = list_entry(curr, struct queue_item_t, list);

        curr = curr->prev;

        delete_file_and_remove_command(queue_item);
    }
}