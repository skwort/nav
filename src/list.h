/**
 * @file list.h
 * @brief List Abstract Data Type (ADT) interface.
 *
 * This file defines the interface for a singly-linked list ADT. The list
 * supports node creation, appending, retrieval, and deletion of nodes, and
 * allows for custom comparison and cleanup functions to be defined for the
 * data stored in the list.
 *
 * The list structure uses dynamically allocated memory, and it's the
 * user's responsibility to provide appropriate cleanup functions for the
 * data when nodes are removed.
 */

#ifndef LIST_H_
#define LIST_H_

/**
 * @brief Structure representing a node in the list.
 */
struct node {
    void *data;
    struct node *next;
};

/**
 * @brief Structure representing the list.
 *
 * The list contains a pointer to the head node and tracks the number of
 * items in the list. It also holds function pointers for comparing
 * and cleaning up data stored in the list.
 */
struct list {
    struct node *head;
    int n_items;

    int (*compare_func)(void *data, void *key);
    int (*cleanup_func)(void *data);
};

/**
 * @brief Creates a new node for the list.
 *
 * This function allocates memory for a new node and returns it via the
 * provided pointer. The node is initialised with `NULL` data and `next`.
 *
 * @param n Double pointer to the node to be created.
 * @return 0 on success, non-zero on failure (e.g., if memory allocation fails).
 */
int list_node_create(struct node **n);

/**
 * @brief Retrieves a node from the list by key.
 *
 * This function searches the list for a node whose data matches the
 * provided key, as determined by the list's `compare_func`. If a matching
 * node is found, it is returned. Otherwise, `NULL` is returned.
 *
 * @param l Pointer to the list.
 * @param key Key used to search for the node (compared against node data).
 * @return Pointer to the found node, or `NULL` if no matching node is found.
 */
struct node *list_get_node(struct list *l, void *key);

/**
 * @brief Appends a node to the end of the list.
 *
 * This function appends the given node to the end of the list. The node's
 * `next` pointer is set to `NULL` and the node is added to the list.
 * The list's item count is incremented.
 *
 * @param l Pointer to the list.
 * @param n Pointer to the node to append.
 * @return 0 on success, non-zero on failure.
 */
int list_append_node(struct list *l, struct node *n);

/**
 * @brief Prepends a node to the start of the list.
 *
 * This function prepends the given node to the start of the list.
 * The list's item count is incremented.
 *
 * @param l Pointer to the list.
 * @param n Pointer to the node to append.
 * @return 0 on success, non-zero on failure.
 */
int list_prepend_node(struct list *l, struct node *n);

/**
 * @brief Deletes a node from the list by key.
 *
 * This function searches the list for a node whose data matches the provided
 * key and removes the node from the list. The node's data is cleaned up
 * using the list's `cleanup_func`, and the node itself is freed. The list's
 * item count is decremented.
 *
 * @param l Pointer to the list.
 * @param key Key used to search for the node (compared against node data).
 * @return 0 on success, non-zero on failure.
 */
int list_delete_node(struct list *l, void *key);

#endif /* LIST_H_ */
