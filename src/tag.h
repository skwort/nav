/**
 * @file tag.h
 * @brief Tag list node storage and utility functions
 *
 * This header defines the `struct tag`, which stores the entries for the 
 * tag list, and the associated struct list comparison and cleanup functions.
 */

#ifndef TAG_H_
#define TAG_H_

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

#endif /* TAG_H_ */
