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
#include <signal.h>

#include "log.h"
#include "commands.h"
#include "list.h"
#include "state.h"
#include "shell.h"
#include "tag.h"
#include "utils.h"

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
    }
}

static inline void setup_initial_state(struct state *state, char *rootdir)
{
    int err;
    struct stat sb;

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

    if (rootdir == NULL) {
        /* Use the default root directory */
        err = snprintf(state->rootdir, 64, "/home/%s/.nav", state->uname);
        if (err >= 108 || err <= 0) {
            LOG_ERR("Unable to set root directory.");
            exit(EXIT_FAILURE);
        }
    } else {
        /* Use the specified root directory */
        err = snprintf(state->rootdir, 108, "%s", rootdir);
        if (err >= 108 || err <= 0) {
            LOG_ERR("Unable to set root directory.");
            exit(EXIT_FAILURE);
        }
    }

    err = fstatat(0, state->rootdir, &sb, 0);
    if (err && errno == ENOENT) {
        LOG_INF("Creating root dir '%s'", state->rootdir);
        err = mkdir(state->rootdir, 0700);
    }

    if (err) {
        LOG_ERR("Error with root dir.");
        exit(EXIT_FAILURE);
    }
    LOG_INF("Root dir is %s", state->rootdir);

    err = snprintf(state->nav_path, 108, "%s/nav.sock", state->rootdir);
    if (err >= 108 || err <= 0) {
        LOG_ERR("Cannot get nav socket path.");
        exit(EXIT_FAILURE);
    }

    sprintf(state->tagfile_path, "%s/tags", state->rootdir);
    read_tag_file(&state->tags, state->tagfile_path);
}

static void register_signal_handlers(void)
{
    struct sigaction sa;
    sa.sa_sigaction = &handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void setup_socket(struct state *state)
{
    int err;
    struct sockaddr_un nav_addr;

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
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] <command> [arguments]\n", program_name);
    printf(
        "Options:\n"
        "  -d <directory>    Specify the directory to use.\n"
        "  -v                Print version.\n");
}

static void parse_args(int argc, char** argv, char **dir)
{
    int opt;

    while ((opt = getopt(argc, argv, "vd:")) != -1) {
        switch (opt) {
        case 'd':
            *dir = optarg;
            break;
        case 'v':
            printf("nav daemon version 0\n");
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char** argv)
{
    struct state *state;
    char *rootdir = NULL;

    parse_args(argc, argv, &rootdir);

    if (init_state()) {
        exit(EXIT_FAILURE);
    }
    state = get_state();

    setup_initial_state(state, rootdir);
    setup_socket(state);
    register_signal_handlers();

    loop(state);

    unlink(state->nav_path);
    close(state->sfd);
    deinit_state();

    return 0;
}