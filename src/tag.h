/**
 * @file tag.h
 * @brief Tag list node storage and utility functions
 *
 * This header defines the `struct tag`, which stores the entries for the 
 * tag list, and the associated struct list comparison and cleanup functions.
 */

#ifndef TAG_H_
#define TAG_H_

#include "list.h"

struct tag {
    char *tag;
    char *path;
};

/**
 * @brief Compares the tag of a tag node with a given key.
 *
 * This function compares the tagstored in a tag node with a given tag key.
 * It is used as a comparison function for list operations (e.g., searching for
 * a tag node).
 *
 * @param data Pointer to the `struct tag` to be compared.
 * @param key Pointer to the key (tag) to compare against.
 * @return 0 if the tags match, 1 otherwise.
 */
int compare_tag_tag(void *data, void *key);

/**
 * @brief Cleans up and deallocates memory for a tag node.
 *
 * This function frees the memory associated with a tag node. This amounts to
 * free the tag and path. It is used as a cleanup function when removing tag
 * nodes from the tag list.
 *
 * @param data Pointer to the `struct tag` to be cleaned up.
 * @return 0 on success.
 */
int cleanup_tag(void *data);

/**
 * @brief Reads tag data from a file and populates the provided tag list.
 *
 * This function opens the specified file at `path` and reads each line,
 * expecting a format of "tag=tag_path". It parses each line into tag and path
 * components, verifies the path, and adds each unique tag-path pair to the
 * given `tags` list. If a tag already exists, the path is updated instead.
 *
 * @param tags Pointer to the `list` structure where parsed tags will be
 *             stored.
 * @param path Pointer to the file path to read tags from.
 * @return 0 on success, 1 if the file cannot be opened or if memory allocation
 *         fails.
 */
int read_tag_file(struct list *tags, char *path);

/**
 * @brief Writes the provided tag list to a file.
 *
 * This function writes each tag-path pair from the `tags` list to the
 * specified file at `path`, in the format "tag=tag_path". Each entry is
 * written on a new line, and an extra newline is added at the end of the file.
 * Existing file contents are overwritten.
 *
 * @param tags Pointer to the `list` structure containing tags to write.
 * @param path Pointer to the file path to write tags to.
 * @return 0 on success, 1 if the file cannot be opened.
 */
int write_tag_file(struct list *tags, char *path);

#endif /* TAG_H_ */
