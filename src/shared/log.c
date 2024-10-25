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

void _log(FILE *file, const char *level, const char *func,
          const char *file_name, const char *fmt, ...)
{
    va_list ap;

    file = (file == NULL) ? stderr : file;

#ifdef PRETTY_LOG
    bold(file);
    fprintf(file, "[%s::%s] [%s]: ", file_name, func, level);
    unbold(file);
#else
    fprintf(file, "[%s::%s] [%s]: ", file_name, func, level);
#endif

    va_start(ap, fmt);
    vfprintf(file, fmt, ap);
    va_end(ap);

    fprintf(file, "\n");
}