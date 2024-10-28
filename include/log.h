/**
 * @file log.h
 * @brief Logging utility for formatted output with timestamp and PID.
 *
 * This header defines the logging interface for formatted output to a given
 * file stream, or `stderr` by default. It provides a convenient way to log
 * messages with customiseable metadata.
 */

#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>

/* If defined, this will enable formatted log outputs using escape codes */
#define PRETTY_LOG

/* Define log levels */
enum log_level {
    LOG_LVL_INF = 0,
    LOG_LVL_ERR,
    LOG_LVL_NONE,
};

/**
 * @brief Sets the internal log level to `level`
 *
 * This function allows for runtime filtering of log messages based on their
 * severity.
 */
void set_log_level(int level);

/**
 * @brief Logs a formatted message to the specified file stream.
 *
 * This internal function logs a message to the specified file stream, or to
 * `stderr` if the file stream is `NULL`. Each log entry includes a timestamp
 * and process ID (PID) before the actual log message.
 */
void _log(FILE *file, int level, const char *func,
          const char *file_name, const char *fmt, ...);

/**
 * @def LOGF(f, ...)
 * @brief Macro to log a message to a specific file.
 *
 * Logs a message to the specified file `f` using a formatted string with 
 * variable arguments. Uses `_log()` internally.
 */
#define LOGF(f, level, ...) _log(f, level, __func__, __FILE__, __VA_ARGS__)

/**
 * @def LOG(...)
 * @brief Macro to log a message to stderr.
 *
 * Logs a message to `stderr` using a formatted string with variable arguments. 
 * Uses `_log()` internally.
 */
#define LOG(level, ...)  _log(NULL, level, __func__, __FILE__, __VA_ARGS__)

/* Extra macros for pre-filled log levels */
#define LOG_INF(...)  _log(NULL, LOG_LVL_INF, __func__, __FILE__, __VA_ARGS__)
#define LOG_ERR(...)  _log(NULL, LOG_LVL_ERR, __func__, __FILE__, __VA_ARGS__)

#endif /* __LOG_H_ */
