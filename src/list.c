/**
 * @file list.c
 * @brief List ADT implementation 
 *
 * This file provides the implementation of the singly-linked list ADT.
 */

#include <stdlib.h>

#include "list.h"

int list_node_create(struct node **n)
{
    *n = malloc(sizeof(struct node));
    if (*n == NULL)
        return 1;

    (*n)->next = NULL;
    return 0;
}

int list_append_node(struct list *l, struct node *n)
{
    /* empty list */
    if (l->head == NULL) {
        l->head = n;
        l->n_items++;
        return 0; 
    }

    /* non-empty list */
    struct node* curr = l->head;
    for (; curr != NULL; curr = curr->next) {
        if (curr->next == NULL) {
            curr->next = n;
            l->n_items++;
            break;
        }
    }

    return 0;
}

int list_prepend_node(struct list *l, struct node *n)
{
    /* empty list */
    if (l->head == NULL) {
        l->head = n;
        l->n_items++;
        return 0; 
    }

    /* non-empty list */
    n->next = l->head; 
    l->head = n;
    l->n_items++;

    return 0;
}

int list_delete_node(struct list *l, void *key)
{
    struct node *curr, *prev;

    /* empty list */
    if (l->head == NULL) {
        return 1;
    }

    /* first item */
    if (!l->compare_func(l->head->data, key)) {
        curr = l->head;
        l->cleanup_func(curr->data);
        l->head = l->head->next;
        free(curr);
        l->n_items--;
        return 0;
    }

    /* not first item */
    curr = l->head;
    prev = NULL;
    for (; curr != NULL; curr = curr->next) {
        if (!l->compare_func(curr->data, key)) {
            if (prev)
                prev->next = curr->next;

            l->cleanup_func(l->head->data);
            free(curr);
            l->n_items--;
            break;
        }

        prev = curr;
    }

    return 0;
}

struct node *list_get_node(struct list *l, void *key)
{
    struct node* curr;

    /* empty list */
    if (l->head == NULL)
        return NULL;

    /* first item */
    if (!l->compare_func(l->head->data, key))
        return l->head;

    /* not first item */
    for (curr = l->head; curr != NULL; curr = curr->next) {
        if (!l->compare_func(curr->data, key))
            return curr;
    }
    
    return NULL;
}