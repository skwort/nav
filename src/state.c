/**
 * @file state.c
 * @brief Implementation of global state management for navd.
 *
 * This file provides the implementation for managing the global state of 
 * the navd daemon. It defines functions for initialising, deinitialising, 
 * and retrieving the singleton state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "state.h"
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
        memset(singleton_state->nav_path, 0, PATH_MAX);
        singleton_state->uname = NULL;
        singleton_state->sfd = -1;

        singleton_state->shells.head = NULL;
        singleton_state->shells.n_items = 0;
        singleton_state->shells.compare_func= NULL;
        singleton_state->shells.cleanup_func = NULL;

        return 0;
    }

    return 1;
}

void deinit_state(void)
{
    free(singleton_state);
}

struct state *get_state(void)
{
    return singleton_state;
}
