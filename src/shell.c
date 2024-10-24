/**
 * @file shell.c
 * @brief Implementation of shell node state storage and utility functions.
 *
 * This file provides the implementation for managing shell nodes in the shell
 * list.
 */

#include <stdlib.h>

#include "shell.h"

int compare_shell_pid(void *data, void *key)
{
    struct shell *s = (struct shell *)data;
    int *pid = (int *)key;

    if (s->pid == *pid) {
        return 0;
    }

    return 1;
}

int cleanup_shell(void *data)
{
    free(data);
    return 0;
}