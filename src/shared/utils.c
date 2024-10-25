/**
 * @file utils.c
 * @brief Implementation of utility functions.
 *
 * This file implements the general purpose utility functions used across the
 * project.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#include "utils.h"
#include "log.h"

char *get_username(void)
{
    uid_t uid;
    struct passwd* pwd;

    uid = getuid();
    pwd = getpwuid(uid);
    if (pwd == NULL) {
        LOG_ERR("User does not exist.");
        return NULL;
    }

    return pwd->pw_name;
}

int get_trailing_whitespace(char *s)
{
    int i = 0;
    char *p = s;

    while (p != NULL) {
        if (*p == '\n' || *p == ' ')
            break;
        i++;
        p++;
    }

    return i;
}

bool valid_path(char *path)
{
    struct stat sb;
    int err;

    err = fstatat(0, path, &sb, 0);
    if (err && errno == ENOENT) {
        LOG_ERR("Path '%s' does not exist.", path);
        return false;
    } else if (err) {
        LOG_ERR("Bad path %s", path);
        return false;
    }

    return true;
}