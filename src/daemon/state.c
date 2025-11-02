/**
 * @file state.c
 * @brief Implementation of global state management for navd.
 *
 * This file provides the implementation for managing the global state of
 * the navd daemon. It defines functions for initialising, deinitialising,
 * and retrieving the singleton state.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "state.h"
#include "list.h"
#include "log.h"

/**
 * @brief Static instance of the global state.
 *
 * This variable holds the singleton instance of the global state, ensuring
 * that only one instance of the state exists throughout the program.
 */
static struct state *singleton_state = NULL;

int init_state(void)
{
    if (singleton_state == NULL) {
        singleton_state = (struct state *)malloc(sizeof(struct state));
        if (singleton_state == NULL) {
            LOG_ERR("init_state: %s", strerror(errno));
            return 1;
        }

        /* Set default state values */
        memset(singleton_state->rootdir, 0, sizeof(singleton_state->rootdir));
        memset(singleton_state->nav_path, 0, PATH_MAX);
        memset(singleton_state->tagfile_path, 0, PATH_MAX);
        singleton_state->uname = NULL;
        singleton_state->sfd = -1;

        /* Setup shell list */
        singleton_state->shells.head = NULL;
        singleton_state->shells.n_items = 0;
        singleton_state->shells.compare_func = NULL;
        singleton_state->shells.cleanup_func = NULL;

        /* Setup tag list */
        singleton_state->tags.head = NULL;
        singleton_state->tags.n_items = 0;
        singleton_state->tags.compare_func = NULL;
        singleton_state->tags.cleanup_func = NULL;

        return 0;
    }

    return 1;
}

void deinit_state(void)
{
    list_delete_all(&singleton_state->shells);
    list_delete_all(&singleton_state->tags);

    free(singleton_state);
}

struct state *get_state(void)
{
    return singleton_state;
}
