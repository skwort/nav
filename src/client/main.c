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

/* Global state variables */
static int sfd;
static char *uname;
static struct sockaddr_un my_addr;
static struct sockaddr_un nav_addr;
static char rootdir[64] = {0};

static void handler(int signo, siginfo_t *info, void* context)
{
    if (strlen(my_addr.sun_path) > 0) {
        unlink(my_addr.sun_path);
    }

    _exit(EXIT_SUCCESS);
}

static void process_command(int argc, char** argv)
{
    char buf[1024] = {0};
    int offset = 0;
    char **ptr = argv + 1;  /* Skip PID */

    while (*ptr != NULL) {
        offset += sprintf(buf + offset,  "%s ", *ptr);
        ptr++;
    }

    send(sfd, buf, strlen(buf), 0);

    memset(buf, 0, sizeof(buf));
    recv(sfd, buf, sizeof(buf), 0);
    printf("%s", buf);

    close(sfd);
    unlink(my_addr.sun_path);

    return;
}

int main(int argc, char** argv)
{
    int opt;
    int err;
    struct sigaction sa;

    /* Register SIGINT handler */
    sa.sa_sigaction = &handler;
    sigaction(SIGINT, &sa, NULL);

    while ((opt = getopt(argc, argv, "vd:")) != -1) {
        switch (opt) {
        case 'd':
            strncpy(rootdir, optarg, sizeof(rootdir));
            break;
        case 'v':
            printf("nav client version 0\n");
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Usage: %s -d <directory> [command]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Usage: %s -d <directory> [command]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uname = get_username();
    if (uname == NULL) {
        LOG_ERR("Invalid user.");
        exit(EXIT_FAILURE);
    }

    /* Setup root directory */
    if (strlen(rootdir) > 0) {
        if (!valid_path(rootdir))
            exit(EXIT_FAILURE);
    } else {
        sprintf(rootdir, "/home/%s/.nav", uname);
    }
    LOG_INF("Using root nav directory '%s'", rootdir);

    nav_addr.sun_family = AF_UNIX;
    snprintf(nav_addr.sun_path, 108, "%s/nav.sock", rootdir);

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        LOG_ERR("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    my_addr.sun_family = AF_UNIX;
    snprintf(my_addr.sun_path, 108, "%s/%s.sock", rootdir, argv[optind]);
    err = bind(sfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
    if (err == -1) {
        LOG_ERR("bind: %s", strerror(errno));
        close(sfd);
        exit(EXIT_FAILURE);
    }

    err = connect(sfd, (struct sockaddr *)&nav_addr, sizeof(nav_addr));
    if (err == -1) {
        LOG_ERR("connect: %s '%s'", strerror(errno), nav_addr.sun_path);
        close(sfd);
        unlink(my_addr.sun_path);
        exit(EXIT_FAILURE);
    }

    argc -= optind;
    argv += optind;
    process_command(argc, argv);

    return 0;
}