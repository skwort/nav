/**
 * @file commands.c
 * @brief Command dispatching and handling functionality.
 *
 * This file contains the implementation of the command dispatching mechanism.
 * It provides functions to register and unregister shells, as well as a 
 * dispatch function that matches command strings to the appropriate handlers.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "log.h"
#include "state.h"
#include "shell.h"

/**
 * @brief Enum representing the available commands.
 *
 * This enum is used to index the command table and map command strings 
 * to their corresponding functions.
 */
enum commands {
    CMD_REGISTER = 0,
    CMD_UNREGISTER,
    CMD_NUM 
};

/* Prototypes */
static void cmd_register(int pid, char *args);
static void cmd_unregister(int pid, char *args);

/**
 * @brief Structure representing a command entry.
 *
 * This structure holds the mapping between a command string and its
 * corresponding function. It is used in the dispatch table to determine which
 * function to call based on the command string.
 */
struct command {
    const char *cmd_name;
    void (*cmd_func)(int pid, char *args);
};

const struct command cmd_table[] = {
    {"register", cmd_register},
    {"unregister", cmd_unregister},
};

void dispatch_command(char *cmd_str, int pid, char *args)
{
    int cmd_len;
    int i;

    for (i = 0; i < CMD_NUM; i++) {
        cmd_len = strlen(cmd_table[i].cmd_name);

        if (strncmp(cmd_str, cmd_table[i].cmd_name, cmd_len) == 0) {
            cmd_table[i].cmd_func(pid, args);
            return;
        }
    }
    LOG_INF("Unknown command: %s", cmd_str);
}

/**
 * @brief Registers a shell with the provided PID.
 *
 * This function handles the registration of a shell with the given PID.
 * It checks if the shell is already registered, allocates necessary
 * resources, and stores the shell data in the global state.
 *
 * @param pid The PID of the shell to register.
 * @param args Additional arguments (currently unused).
 */
static void cmd_register(int pid, char *args)
{
    struct state *state;
    struct shell *shell_data;
    struct node *shell_node;
    char buf[100] = {0};
    
    state = get_state();

    LOG_INF("shell at PID=%d wants to register", pid);

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node) {
        LOG_INF("shell %d already registered", pid);
        return;
    }

    if (list_node_create(&shell_node)) {
        LOG_ERR("shell node create failed");
        return;
    }

    shell_data = (struct shell *)malloc(sizeof(struct shell));
    if (shell_data == NULL) {
        LOG_ERR("shell data malloc create failed");
        return;
    }

    shell_data->pid = pid;
    shell_data->sock_addr.sun_family = AF_UNIX;
    snprintf(shell_data->sock_addr.sun_path, 108,
        "/home/%s/.nav/%d.sock", state->uname, pid); 

    shell_node->data = shell_data;
    list_append_node(&state->shells, shell_node);

    /* Send registration message */
    snprintf(buf, 100, "%d register OK\n", pid);
    sendto(state->sfd, buf, 100, 0, (struct sockaddr *)&shell_data->sock_addr,
           sizeof(shell_data->sock_addr));

    LOG_INF("shell %d registered", pid);
}

/**
 * @brief Unregisters a shell with the provided PID.
 *
 * This function handles the unregistration of a shell with the given PID.
 * It checks if the shell exists in the global state, removes it from the
 * shell list, and deallocates associated resources.
 *
 * @param pid The PID of the shell to unregister.
 * @param args Additional arguments (currently unused).
 */
static void cmd_unregister(int pid, char *args)
{
    struct state *state;
    struct node *shell_node;

    LOG_INF("shell at PID=%d wants to unregister", pid);

    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }

    if (list_delete_node(&state->shells, &pid)) {
        LOG_ERR("shell node delete failed");
        return;
    }
    LOG_INF("shell %d unregistered", pid);
}