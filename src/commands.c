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
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "list.h"
#include "log.h"
#include "state.h"
#include "shell.h"
#include "tag.h"
#include "utils.h"

/**
 * @brief Enum representing the available commands.
 *
 * This enum is used to index the command table and map command strings 
 * to their corresponding functions.
 */
enum commands {
    CMD_REGISTER = 0,
    CMD_UNREGISTER,
    CMD_ADD,
    CMD_DELETE,
    CMD_SHOW,
    CMD_GET,
    CMD_NUM 
};

/* Prototypes */
static void cmd_register(int pid, char *args);
static void cmd_unregister(int pid, char *args);
static void cmd_add(int pid, char *args);
static void cmd_delete(int pid, char *args);
static void cmd_show(int pid, char *args);
static void cmd_get(int pid, char *args);

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
    {"add", cmd_add},
    {"delete", cmd_delete},
    {"show", cmd_show},
    {"get", cmd_get},
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
    sendto(state->sfd, buf, 100, 0,
           (struct sockaddr *)&shell_data->sock_addr,
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
    struct shell *shell_data;
    struct sockaddr_un shell_addr;

    LOG_INF("shell at PID=%d wants to unregister", pid);

    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }
    shell_data = (struct shell *)shell_node->data;

    memcpy(&shell_addr, &shell_data->sock_addr, sizeof(shell_addr));

    if (list_delete_node(&state->shells, &pid)) {
        LOG_ERR("shell node delete failed");
        return;
    }

    LOG_INF("shell %d unregistered", pid);
    sendto(state->sfd, "OK\n", 3, 0,
           (struct sockaddr *)&shell_addr,
           sizeof(shell_addr));
}



static void cmd_add(int pid, char *args)
{
    char *line = NULL, *token = NULL, *tag = NULL, *path = NULL;
    char *saveptr = NULL;
    int j;
    struct state *state;
    struct node *tag_node;
    struct tag *tag_data;
    struct node *shell_node;
    struct shell *shell_data;

    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }
    shell_data = (struct shell *)shell_node->data;

    /* Extract args */
    for (j = 1, line = args; ; j++, line = NULL) {
        token = strtok_r(line, " ", &saveptr);
        if (token == NULL)
            break;

        if (j == 1) {
            tag = strndup(token, get_trailing_whitespace(token));
        } else if (j == 2) {
            path = strndup(token, get_trailing_whitespace(token));
        } else {
            LOG_ERR("Too many tokens");
            goto freeargs;
        }
    }

    if (path == NULL) {
        LOG_ERR("Too few tokens");
        goto freeargs;
    }

    if (!valid_path(path)) {
        goto freeargs;
    }

    /* Check if it already exists */
    tag_node = list_get_node(&state->tags, tag);
    if (tag_node != NULL) {
        LOG_INF("Tag '%s' already exists. Updating.", tag);
        free(((struct tag *)tag_node->data)->path);
        ((struct tag *)tag_node->data)->path = path;
        goto end;
    }

    /* Create the tag and add it to the list*/
    if (list_node_create(&tag_node)) {
        LOG_ERR("tag node create failed");
        goto freeargs;
    }

    tag_data = (struct tag*)malloc(sizeof(struct tag));
    if (tag_data == NULL) {
        LOG_ERR("tag data malloc create failed");
        goto freeargs;
    }

    tag_data->tag = tag;
    tag_data->path = path;
    tag_node->data = tag_data;
    list_append_node(&state->tags, tag_node);

end:
    LOG_INF("Tag %s --> %s added.", tag, path);
    sendto(state->sfd, "OK\n", 3, 0,
           (struct sockaddr *)&shell_data->sock_addr,
           sizeof(shell_data->sock_addr));
    return;

freeargs:
    free(tag);
    free(path);

    sendto(state->sfd, "BAD\n", 4, 0,
           (struct sockaddr *)&shell_data->sock_addr,
           sizeof(shell_data->sock_addr));
    return;
}

static void cmd_delete(int pid, char *args)
{
    char *line = NULL, *token = NULL, *tag = NULL;
    char *saveptr = NULL;
    struct state *state;
    struct node *tag_node;
    struct node *shell_node;
    struct shell *shell_data;

    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }
    shell_data = (struct shell *)shell_node->data;

    line = args;
    token = strtok_r(line, " ", &saveptr);
    if (token == NULL) {
        LOG_ERR("Too few tokens");
        return;
    }

    tag = strndup(token, get_trailing_whitespace(token));

   /* Check if tag exists */
    tag_node = list_get_node(&state->tags, tag);
    if (tag_node == NULL) {
        LOG_INF("Tag '%s' does not exist.", tag);
        sendto(state->sfd, "BAD\n", 4, 0,
            (struct sockaddr *)&shell_data->sock_addr,
            sizeof(shell_data->sock_addr));
    } else {
        list_delete_node(&state->tags, tag);
        LOG_INF("Tag '%s' deleted.", tag);
        sendto(state->sfd, "OK\n", 3, 0,
            (struct sockaddr *)&shell_data->sock_addr,
            sizeof(shell_data->sock_addr));
    }

    free(tag);
    return;
}

static void cmd_show(int pid, char *args)
{
    struct state *state;
    struct node *tag_node;
    struct node *shell_node;
    struct shell *shell_data;
    struct tag *tag_data;
    char buf[256] = {0};
    int offset = 0;
    
    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }

    shell_data = (struct shell *)shell_node->data;

    tag_node = state->tags.head;
    while (tag_node != NULL) {
        tag_data = (struct tag *)tag_node->data;

        offset += sprintf(buf + offset,  "%s --> %s\n", tag_data->tag,
                          tag_data->path);
        tag_node = tag_node->next;
    }

    sendto(state->sfd, buf, 256, 0,
           (struct sockaddr *)&shell_data->sock_addr,
           sizeof(shell_data->sock_addr));
}

static void cmd_get(int pid, char *args)
{
    char *line = NULL, *token = NULL, *tag = NULL;
    char *saveptr = NULL;
    struct state *state;
    struct node *tag_node;
    struct node *shell_node;
    struct shell *shell_data;
    struct tag *tag_data;
    char buf[100] = {0};
    
    state = get_state();

    shell_node = list_get_node(&state->shells, &pid);
    if (shell_node == NULL) {
        LOG_ERR("shell %d does not exist", pid);
        return;
    }
    shell_data = (struct shell *)shell_node->data;

    line = args;
    token = strtok_r(line, " ", &saveptr);
    if (token == NULL) {
        LOG_ERR("Too few tokens.");
        return;
    }

    tag = strndup(token, get_trailing_whitespace(token));

    /* Check if tag exists */
    tag_node = list_get_node(&state->tags, tag);
    if (tag_node == NULL) {
        LOG_INF("Tag '%s' does not exist.", tag);
        sendto(state->sfd, "BAD\n", 4, 0,
            (struct sockaddr *)&shell_data->sock_addr,
            sizeof(shell_data->sock_addr));
    } else {
        tag_data = (struct tag *)tag_node->data;
        sprintf(buf,  "%s\n", tag_data->path);
        sendto(state->sfd, buf, strlen(buf), 0,
            (struct sockaddr *)&shell_data->sock_addr,
            sizeof(shell_data->sock_addr));
    }

    free(tag);
    return;
}
