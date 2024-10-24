/**
 * @file tag.c
 * @brief Implementation of tag storage for navd.
 *
 * This file provides the implementation of the tag list compare and cleanup
 * functions.
 */

#include <stdlib.h>
#include <string.h>

#include "tag.h"

int compare_tag_tag(void *data, void *key)
{
    struct tag *tag = (struct tag *)data;

    if (strcmp(tag->tag, key) == 0)
        return 0;

    return 1;
}

int cleanup_tag(void *data)
{
    struct tag *tag = (struct tag *)data;

    free(tag->tag);
    free(tag->path);

    return 0;
}