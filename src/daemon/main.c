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

/* The maximum size of the socket paths. This value is limited by the size of
 * sockaddr_un->sun_path . */
#define SOCKADDR_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

#define CONFIG_DIR_ENV_VAR "NAV_CONFIG_DIR"
#define CACHE_DIR_ENV_VAR  "NAV_CACHE_DIR"

#define DEFAULT_CONFIG_DIR  "/home/%s/.config/nav"
#define DEFAULT_CACHE_DIR   "/home/%s/.cache/nav"
#define DEFAULT_SOCKET_FILE "nav.sock"
#define DEFAULT_TAG_FILE    "tags"

void handler(int signo, siginfo_t *info, void *context)
{
    struct state *state = get_state();

    /* Remove the nav socket file on shutdown */
    if (strlen(state->nav_socket_path) > 0) {
        unlink(state->nav_socket_path);
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

static int setup_directory(char *dest, size_t dest_size, const char *env_var,
                           const char *default_fmt, const char *username,
                           const char *dir_name)
{
    int err;
    struct stat sb;
    char *temp_env;

    /* Extract from environment or use default */
    temp_env = getenv(env_var);
    if (temp_env) {
        /* Check for truncation */
        if (strlen(temp_env) >= dest_size) {
            LOG_ERR("Path too long for %s: '%s'", dir_name, temp_env);
            return -1;
        }
        strncpy(dest, temp_env, dest_size);
        dest[dest_size - 1] = '\0';
    } else {
        err = snprintf(dest, dest_size, default_fmt, username);
        if (err >= (int)dest_size || err < 0) {
            LOG_ERR("Failed to format default path for %s", dir_name);
            return -1;
        }
    }

    LOG_INF("Loaded %s '%s'", dir_name, dest);

    /* Create the directory if it doesn't exist */
    err = fstatat(0, dest, &sb, 0);
    if (err && errno == ENOENT) {
        LOG_INF("Creating %s '%s'", dir_name, dest);
        err = mkdir(dest, 0700);
    }

    if (err) {
        LOG_ERR("Error with %s.", dir_name);
        return -1;
    }

    return 0;
}

static inline void setup_initial_state(struct state *state)
{
    int err;

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

    /* Setup config directory */
    if (setup_directory(state->config_dir, sizeof(state->config_dir),
                        CONFIG_DIR_ENV_VAR, DEFAULT_CONFIG_DIR, state->uname,
                        "config dir") != 0) {
        exit(EXIT_FAILURE);
    }

    /* Setup cache directory */
    if (setup_directory(state->cache_dir, sizeof(state->cache_dir),
                        CACHE_DIR_ENV_VAR, DEFAULT_CACHE_DIR, state->uname,
                        "cache dir") != 0) {
        exit(EXIT_FAILURE);
    }

    err = snprintf(state->nav_socket_path, sizeof(state->nav_socket_path),
                   "%s/" DEFAULT_SOCKET_FILE, state->cache_dir);
    if (err >= SOCKADDR_PATH_MAX || err <= 0) {
        LOG_ERR("Cannot get nav socket path.");
        exit(EXIT_FAILURE);
    }

    snprintf(state->tagfile_path, sizeof(state->tagfile_path),
             "%s/" DEFAULT_TAG_FILE, state->config_dir);
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
    unlink(state->nav_socket_path);
    nav_addr.sun_family = AF_UNIX;
    memcpy(nav_addr.sun_path, state->nav_socket_path, SOCKADDR_PATH_MAX);
    err = bind(state->sfd, (struct sockaddr *)&nav_addr, sizeof(nav_addr));
    if (err == -1) {
        LOG_ERR("bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    LOG_INF("Socket created: %s", state->nav_socket_path);
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] <command> [arguments]\n", program_name);
    printf("Options:\n"
           "  -v                Print version.\n");
}

static void parse_args(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
        case 'v':
            printf("nav daemon version 0\n");
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv)
{
    struct state *state;

    parse_args(argc, argv);

    if (init_state()) {
        exit(EXIT_FAILURE);
    }
    state = get_state();

    setup_initial_state(state);
    setup_socket(state);
    register_signal_handlers();

    loop(state);

    unlink(state->nav_socket_path);
    close(state->sfd);
    deinit_state();

    return 0;
}
