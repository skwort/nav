#include <stdio.h>
#include <stdlib.h>
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
    char **ptr = argv + 1;

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
    int err;
    struct sigaction sa;

    /* Register SIGINT handler */
    sa.sa_sigaction = &handler;
    sigaction(SIGINT, &sa, NULL);

    uname = get_username();
    if (uname == NULL) {
        LOG_ERR("Invalid user.");
        exit(EXIT_FAILURE);
    }

    nav_addr.sun_family = AF_UNIX;
    sprintf(nav_addr.sun_path, "/home/%s/.nav/nav.sock", uname);

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        LOG_ERR("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    my_addr.sun_family = AF_UNIX;
    sprintf(my_addr.sun_path, "/home/sam/.nav/%s.sock", argv[1]);
    err = bind(sfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
    if (err == -1) {
        LOG_ERR("bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    err = connect(sfd, (struct sockaddr *)&nav_addr, sizeof(nav_addr));
    if (err == -1) {
        LOG_ERR("connect: %s '%s'", strerror(errno), nav_addr.sun_path);
        exit(EXIT_FAILURE);
    }

    process_command(argc, argv);

    return 0;
}