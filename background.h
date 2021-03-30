
#include <stddef.h>
#include <stdbool.h>

#ifndef BACKGROUND_H
#define BACKGROUND_H


int is_background_command(char *cmdline);

void background_command_handler(char *cmdline);

#endif
