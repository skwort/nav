/**
 * @file utils.h
 * @brief Utility functions 
 *
 * This header defines general purpose utility functions used across the
 * project.
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>

/**
 * @brief Retrieves the username of the current user.
 *
 * This function fetches the username associated with the current user ID.
 * It uses `getuid()` to obtain the user ID and `getpwuid()` to look up
 * the corresponding password entry. If the user ID is invalid or not found,
 * an error is logged, and `NULL` is returned.
 *
 * @return A pointer to the username string if successful, or `NULL` if the
 */
char *get_username(void);

/**
 * @brief Get the index of the first trailing whitespace in a string.
 *
 * This function searches for the first occurrence of a newline or space
 * character in a given string. It returns the index of this character if found,
 * or the string's length if no trailing whitespace is present.
 *
 * @param s Pointer to the input string to be analyzed.
 * @return Index of the first trailing whitespace character if found;
 *         otherwise, the length of the string.
 */
int get_trailing_whitespace(char *s);

/**
 * @brief Validate a path to ensure it exists.
 *
 * This function checks if a specified file or directory path exists
 * by performing a `fstatat` system call on the provided path.
 * Logs an error if the path is invalid or does not exist.
 *
 * @param path Pointer to the string containing the path to validate.
 * @return `true` if the path exists; `false` if it does not or
 *         if there is an error accessing the path.
 */
bool valid_path(char *path);

#endif /* UTILS_H_ */