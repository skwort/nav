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

static void sigint_handler(int signo, siginfo_t *info, void* context)
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

static void process_command(int argc, char** argv)
{
    char buf[1024] = {0};
    int offset = 0;
    char **ptr = argv;

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

void setup_socket(char *pid)
{
    int err;

    nav_addr.sun_family = AF_UNIX;
    snprintf(nav_addr.sun_path, 108, "%s/nav.sock", rootdir);

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        LOG_ERR("socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    my_addr.sun_family = AF_UNIX;
    snprintf(my_addr.sun_path, 108, "%s/%s.sock", rootdir, pid);

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
}

void print_usage(const char *program_name) {
    printf("Usage: %s [options] <command> [arguments]\n", program_name);
    printf(
        "Options:\n"
        "  -d <directory>    Specify the directory to use.\n"
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

int main(int argc, char** argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "vd:")) != -1) {
        switch (opt) {
        case 'd':
            strncpy(rootdir, optarg, sizeof(rootdir));
            break;
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

    register_handlers();
    setup_socket(argv[optind]);    

    argc -= optind;
    argv += optind;
    process_command(argc, argv);

    return 0;
}