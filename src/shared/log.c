/**
 * @file log.c
 * @brief Implementation of logging utility for formatted output.
 *
 * This file provides the implementation of a simple logging utility, which logs
 * messages to a given file or `stderr` by default. The prefixed metadata can
 * be configured by modifying the `_log()` function.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"

static int log_level = LOG_LVL_INF;

static char log_level_strings[][6] = {"INFO", "ERROR"};

#ifdef PRETTY_LOG
static inline void bold(FILE *file)
{
    fprintf(file, "\033[1m");
}

static inline void unbold(FILE *file)
{
    fprintf(file, "\033[0m");
}
#endif

void set_log_level(int level)
{
    log_level = level;
}

void _log(FILE *file, int level, const char *func, const char *file_name,
          const char *fmt, ...)
{
    va_list ap;

    if (level < log_level || level < 0 || level >= LOG_LVL_NONE) {
        return;
    }

    file = (file == NULL) ? stderr : file;

#ifdef PRETTY_LOG
    bold(file);
    fprintf(file, "[%s::%s] [%s]: ", file_name, func, log_level_strings[level]);
    unbold(file);
#else
    fprintf(file, "[%s::%s] [%s]: ", file_name, func, log_level_strings[level]);
#endif

    va_start(ap, fmt);
    vfprintf(file, fmt, ap);
    va_end(ap);

    fprintf(file, "\n");
}
