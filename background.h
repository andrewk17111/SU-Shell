/**
 * @file: background.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file for shell queue functions for background processesing
 * 
 * Function signatures used by the shell to interact with the queue feature of the shell.
 * Functions include validators, queueing and dequeing commands, seeing that status and viewing
 * output of a command.
 * 
 * There is a signal handler used by the queue to handle dequeuing the next process. The shell
 * registers the handler on initialization.
 */ 

#include <stddef.h>
#include <stdbool.h>
#include <signal.h>

#ifndef BACKGROUND_H
#define BACKGROUND_H


// structure that wraps each command that is in the queue
struct queue_item_t {
    pid_t pid;
    int job_id;
    char *outfile;
    bool is_complete;

    struct command_t *command;
    struct list_head list;
};

/**
 * Checks that no redirection or piping is present
 * 
 * @param command: command to validate
 */ 
bool is_valid_background_command(struct command_t *command);

/**
 * Checks if a given command is currently in the queue. This is used when doing any
 * cleanup so we avoid double freeing a pointer.
 * 
 * @param command: command to search for in queue
 */ 
bool is_command_in_queue(struct command_t *command);

/**
 * Sets background command's stdin and stdout. Stdin is closed
 * and stdout always goes to a unique temp file. This configuration
 * is stored in the command struct prior to adding it to the queue
 * 
 * @param command: command to set stdin and stdout fields
 * @return status of setup success
 */ 
int set_command_channels(struct command_t *command);

/**
 * Initialized queue_item struct and adds the item to the back of the command queue
 * 
 * @param command: command to add to queue
 */ 
void add_to_queue(struct command_t *command);

/**
 * Prints the status of all commands to the console except the command that is
 * currently being executed in the background
 */ 
void print_all_job_status();

/**
 * Call back function to handle all signals registered in main shell function. 
 * Our shell registers this function to handle SIGCHLD in order to determine if jobs
 * from the queue have been completed and the next should be run. 
 * 
 * This function is also used to confirm if a job in the background was cancelled successfully.
 * 
 * @param signal: signal id
 */ 
void sig_handler(int signal);

/**
 * Prints the output of the command with the specified job id if the command is complete. 
 * The output of the command is stored in a temporary file and the contents is printed to 
 * the console. Once commands output is viewed, the command is removed from queue and file
 * deleted.
 * 
 * @param job_id: id of job to print output file and remove from queue
 */ 
void print_output_and_remove(int job_id);

/**
 * Attempts to cancel command if not yet complete. If not yet complete the command is removed
 * from the queue and deleted. Otherwise the function returns an error signaling that the command
 * has already been completed and cannot be executed.
 * 
 * @param job_id: id of job to attempt cancel
 * @return: status of cancel attempt
 */ 
void attempt_cancel_command(int job_id);

/**
 * Removes all items from queue and frees memory when exiting the shell
 */ 
void queue_cleanup();

#endif
