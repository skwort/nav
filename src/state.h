/**
 * @file state.h
 * @brief Global state management for navd.
 *
 * This header defines the `struct state`, which stores the global state for
 * the navd daemon. The state is managed as a singleton and can be accessed and
 * modified using the provided functions.
 */

#ifndef STATE_H_
#define STATE_H_

#include <limits.h>
#include "list.h"

/**
 * @brief Structure representing the global state of navd.
 */
struct state {
    char        nav_path[PATH_MAX];
    char        *uname;
    int         sfd;

    struct list shells;
};

/**
 * @brief Retrieves the singleton instance of the global state.
 *
 * This function returns the singleton instance of the global state. If the 
 * state has not been initialised, it will return `NULL`.
 *
 * @return Pointer to the global state singleton.
 */
struct state *get_state(void);

/**
 * @brief Initialises the global state.
 *
 * This function allocates memory for the singleton state instance and 
 * initialises the default values for all state fields. It should be called 
 * before any access to the state.
 *
 * @return 0 on success, non-zero on failure.
 */
int init_state(void);

/**
 * @brief Deinitialises the global state and frees resources.
 *
 * This function frees the memory allocated for the global state and cleans up 
 * any associated resources.
 */
void deinit_state(void);

#endif /* STATE_H_ */
