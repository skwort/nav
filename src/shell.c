/**
 * @file shell.c
 * @brief Implementation of shell node state storage and utility functions.
 *
 * This file provides the implementation for managing shell nodes in the shell
 * list.
 */

#include <stdlib.h>

#include "shell.h"
#include "list.h"

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
    struct shell *s = (struct shell *)data;
    struct action *a;

    while (s->actions.n_items > 0) {
        a = (struct action *)s->actions.head->data;
        list_delete_node(&s->actions, a->path);
    }

    free(data);
    return 0;
}

int compare_action_path(void *data, void *key)
{
    struct action *a = (struct action *)data;
    char *path = (char *)key;

    if (!strcmp(a->path, path))
        return 0;

    return 1;
}

int cleanup_action(void *data)
{
    struct action *a = (struct action *)data;

    free(a->path);
    free(data);

    return 0;
}
