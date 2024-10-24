#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <pwd.h>
#include <signal.h>

#include "log.h"
#include "commands.h"
#include "list.h"
#include "state.h"
#include "shell.h"
#include "tag.h"

char *get_username(void)
{
    uid_t uid;
    struct passwd* pwd;

    /* Need to get name of caller to get local dir */
    uid = getuid();
    pwd = getpwuid(uid);
    if (pwd == NULL) {
        LOG_ERR("User does not exist.");
        return NULL;
    }

    return pwd->pw_name;
}

static int create_navdir(char *path)
{
    return mkdir(path, 0700);
}

int get_navpath(char *uname, char* path)
{
    int err;
    struct stat sb;
    char dir[108] = {0};

    /* Generate the top-level nav path */
    err = snprintf(dir, 108, "/home/%s/.nav", uname);
    if (err >= 108 || err <= 0) {
        LOG_ERR("Invalid username or path too long. Cannot get nav path.");
        return EINVAL;
    }

    /* Check it exists */
    err = fstatat(0, dir, &sb, 0);
    if (err && errno == ENOENT) {
        LOG_INF("Creating navdir.");
        err = create_navdir(dir);
    }

    if (err) {
        LOG_ERR("Error getting nav path.");
        return 1;
    }

    /* Make the socket path */
    err = snprintf(path, 108, "%s/nav.sock", dir);
    if (err >= 108 || err <= 0) {
        LOG_ERR("Cannot get nav socket path.");
        return EINVAL;
    }

    return 0;
}

void handler(int signo, siginfo_t *info, void* context)
{
    struct state *state = get_state();

    if (strlen(state->nav_path) > 0) {
        unlink(state->nav_path);
    }

    deinit_state();

    _exit(EXIT_SUCCESS);
}

void loop(struct state *state)
{
    int nbytes = 0;
    char buf[100] = {0};
    char *line, *pid_str, *cmd_str, *args;
    char *saveptr;
    int pid;

    while (true) {
        nbytes = recv(state->sfd, buf, 100, 0);
        if (nbytes > 0) {
            line = buf;
            pid_str = strtok_r(line, " ", &saveptr);
            if (pid_str == NULL) {
                LOG_ERR("parser: Invalid pid arg.");
                continue;
            }

            errno = 0;
            pid = strtol(pid_str, NULL, 10);
            if (errno) {
                LOG_ERR("strol: %s", strerror(errno));
                break;
            }

            cmd_str = strtok_r(NULL, " ", &saveptr);
            if (cmd_str == NULL) {
                LOG_ERR("parser: Invalid command.");
                continue;
            }

            args = strtok_r(NULL, "", &saveptr);

            dispatch_command(cmd_str, pid, args);
        }
        sleep(1);
    }
}

int main(int arc, char** argv)
{
    int err;
    struct state *state;
    struct sockaddr_un nav_addr;

    /* Register SIGINT handler */
    struct sigaction sa;
    sa.sa_sigaction = &handler;
    sigaction(SIGINT, &sa, NULL);

    /* Setup state */
    if (init_state()) {
        exit(EXIT_FAILURE);
    }
    state = get_state();
    state->shells.compare_func = compare_shell_pid;
    state->shells.cleanup_func = cleanup_shell;
    state->tags.compare_func = compare_tag_tag;
    state->tags.cleanup_func = cleanup_tag;

    state->uname = get_username();
    if (state->uname == NULL) {
        LOG_ERR("Invalid user.");
        exit(EXIT_FAILURE);
    }
    LOG_INF("User is %s", state->uname);

    err = get_navpath(state->uname, state->nav_path);
    if (err == -1) {
        LOG_ERR("Unable to get navpath.");
        exit(EXIT_FAILURE);
    }
    LOG_INF("Path is %s", state->nav_path);

    /* Create datagram socket for receiving messages from shells */
    state->sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (state->sfd == -1) {
        LOG_ERR("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Bind socket, setting default dest addr */
    nav_addr.sun_family = AF_UNIX;
    memcpy(nav_addr.sun_path, state->nav_path, 108);
    err = bind(state->sfd, (struct sockaddr *) &nav_addr, sizeof(nav_addr));
    if (err == -1) {
        LOG_ERR("bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    loop(state);

    /* Remove socket */
    unlink(state->nav_path);
    deinit_state();

    return 0;
}