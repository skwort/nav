/**
 * @file shell.h
 * @brief Shell node state storage and utility functions.
 *
 * This header defines the `struct shell` used to store state for each shell in
 * the shell list. It also provides function declarations for comparing shell 
 * nodes by PID and for cleaning up shell data.
 */

#ifndef SHELL_H_
#define SHELL_H_

#include <sys/un.h>

/**
 * @brief Structure representing a shell node.
 *
 * This structure stores information about a shell registered with navd.
 */
struct shell {
    int pid;
    struct sockaddr_un sock_addr;
};

/**
 * @brief Compares the PID of a shell node with a given key.
 *
 * This function compares the PID stored in a shell node with a given PID key.
 * It is used as a comparison function for list operations (e.g., searching for
 * a shell node).
 *
 * @param data Pointer to the `struct shell` to be compared.
 * @param key Pointer to the key (PID) to compare against.
 * @return 0 if the PIDs match, 1 otherwise.
 */
int compare_shell_pid(void *data, void *key);

/**
 * @brief Cleans up and deallocates memory for a shell node.
 *
 * This function frees the memory associated with a shell node. It is used as a 
 * cleanup function when removing shell nodes from the list.
 *
 * @param data Pointer to the `struct shell` to be cleaned up.
 * @return 0 on success.
 */
int cleanup_shell(void *data);

#endif /* SHELL_H_ */
