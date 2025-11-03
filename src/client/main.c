#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "utils.h"
#include "log.h"

#define CACHE_DIR_ENV_VAR   "NAV_CACHE_DIR"
#define DEFAULT_CACHE_DIR   "/home/%s/.cache/nav"
#define DEFAULT_SOCKET_FILE "nav.sock"

/* Global state variables */
static int sfd;
static struct sockaddr_un my_addr;
static struct sockaddr_un nav_addr;

static char cache_dir[95] = {0};

static void sigint_handler(int signo, siginfo_t *info, void *context)
{
    if (strlen(my_addr.sun_path) > 0) {
        unlink(my_addr.sun_path);
    }

    _exit(EXIT_SUCCESS);
}

void register_handlers(void)
{
    struct sigaction sa;

    sa.sa_sigaction = &sigint_handler;
    sigaction(SIGINT, &sa, NULL);
}

static void process_command(int argc, char **argv)
{
    char buf[1024] = {0};
    int offset = 0;
    int err = 0;
    char **ptr = argv;

    while (*ptr != NULL) {
        offset += sprintf(buf + offset, "%s ", *ptr);
        ptr++;
    }

    send(sfd, buf, strlen(buf), 0);

    memset(buf, 0, sizeof(buf));
    err = recv(sfd, buf, sizeof(buf), 0);
    if (err == -1) {
        LOG_ERR("recv: %s", strerror(errno));
        close(sfd);
        unlink(my_addr.sun_path);
        exit(EXIT_FAILURE);
    }

    printf("%s", buf);

    close(sfd);
    unlink(my_addr.sun_path);

    return;
}

void setup_socket(char *pid)
{
    int err;
    struct timeval timeval;

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        LOG_ERR("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Set 0.05 second timeout on receive */
    timeval.tv_sec = 0;
    timeval.tv_usec = 50000;
    err = setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval));
    if (err == -1) {
        LOG_ERR("setsockopt: %s", strerror(errno));
        close(sfd);
        exit(EXIT_FAILURE);
    }

    my_addr.sun_family = AF_UNIX;
    snprintf(my_addr.sun_path, 108, "%s/%s.sock", cache_dir, pid);

    err = bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err == -1) {
        LOG_ERR("bind: %s", strerror(errno));
        close(sfd);
        exit(EXIT_FAILURE);
    }

    nav_addr.sun_family = AF_UNIX;
    snprintf(nav_addr.sun_path, 108, "%s/nav.sock", cache_dir);

    err = connect(sfd, (struct sockaddr *)&nav_addr, sizeof(nav_addr));
    if (err == -1) {
        LOG_ERR("connect: %s '%s'", strerror(errno), nav_addr.sun_path);
        close(sfd);
        unlink(my_addr.sun_path);
        exit(EXIT_FAILURE);
    }
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] <command> [arguments]\n", program_name);
    printf("Options:\n"
           "  -v                Print version.\n"
           "\n"
           "Note: A PID must prefix all commands shown below. Use $$ in bash.\n"
           "\n"
           "Commands:\n"
           "  get [tag]         Retrieve the path for the the specified tag.\n"
           "  add [tag] [path]  Add a new tag-path association.\n"
           "  delete [tag]      Remove the specified tag.\n"
           "  show              Show all tag-path associations.\n"
           "  push              Save an action to the the action stack.\n"
           "  pop               Get the last action from the action stack.\n"
           "  actions           List all recorded actions.\n");
}

int main(int argc, char **argv)
{
    int opt;
    int err;
    char *uname;
    char *temp_env;

    while ((opt = getopt(argc, argv, "vd:")) != -1) {
        switch (opt) {
        case 'v':
            printf("nav client version 0\n");
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    temp_env = getenv(CACHE_DIR_ENV_VAR);
    if (temp_env) {
        /* Check for truncation */
        if (strlen(temp_env) >= sizeof(cache_dir)) {
            LOG_ERR("Path too long for cache dir: '%s'", temp_env);
            return -1;
        }
        strncpy(cache_dir, temp_env, sizeof(cache_dir));
        cache_dir[sizeof(cache_dir) - 1] = '\0';
    } else {
        uname = get_username();
        if (uname == NULL) {
            LOG_ERR("Invalid user.");
            exit(EXIT_FAILURE);
        }
        err = snprintf(cache_dir, sizeof(cache_dir), DEFAULT_CACHE_DIR, uname);
        if (err >= sizeof(cache_dir) || err < 0) {
            LOG_ERR("Failed to format default path for cache dir");
            return -1;
        }
    }
    LOG_INF("Using cache directory '%s'", cache_dir);

    register_handlers();
    setup_socket(argv[optind]);

    argc -= optind;
    argv += optind;
    process_command(argc, argv);

    return 0;
}
