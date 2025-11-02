/**
 * @file tag.c
 * @brief Implementation of tag storage for navd.
 *
 * This file provides the implementation of the tag list compare and cleanup
 * functions and reading and writing of tag files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tag.h"
#include "list.h"
#include "log.h"
#include "utils.h"

int compare_tag_tag(void *data, void *key)
{
    struct tag *tag = (struct tag *)data;

    if (strcmp(tag->tag, key) == 0) {
        return 0;
    }

    return 1;
}

int cleanup_tag(void *data)
{
    struct tag *tag = (struct tag *)data;

    free(tag->tag);
    free(tag->path);

    return 0;
}

int read_tag_file(struct list *tags, char *path)
{
    char *saveptr, *token, *tag = NULL, *tag_path = NULL;
    char line[256] = {0};
    struct node *tag_node;
    struct tag *tag_data;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        LOG_ERR("Unable to open file at %s", path);
        return 1;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strcmp(line, "\n") == 0) {
            break;
        }

        saveptr = NULL;

        token = strtok_r(line, "=", &saveptr);
        if (token == NULL) {
            LOG_ERR("No tag in token.");
            continue;
        }
        tag = strndup(token, get_trailing_whitespace(token));

        token = strtok_r(NULL, "", &saveptr);
        if (token == NULL) {
            LOG_ERR("No tag path in token.");
            free(tag);
            continue;
        }
        tag_path = strndup(token, get_trailing_whitespace(token));

        tag_node = list_get_node(tags, tag);
        if (tag_node != NULL) {
            LOG_INF("Tag '%s' already exists. Updating.", tag);
            ((struct tag *)tag_node->data)->path = tag_path;
            continue;
        }

        if (!valid_path(tag_path)) {
            free(tag);
            free(tag_path);
            continue;
        }

        /* Create the tag and add it to the list*/
        if (list_node_create(&tag_node)) {
            LOG_ERR("tag node create failed");
            free(tag);
            free(tag_path);
            continue;
        }

        tag_data = (struct tag *)malloc(sizeof(struct tag));
        if (tag_data == NULL) {
            LOG_ERR("tag data malloc create failed");
            free(tag);
            free(tag_path);
            continue;
        }

        tag_data->tag = tag;
        tag_data->path = tag_path;
        tag_node->data = tag_data;
        list_append_node(tags, tag_node);

        LOG_INF("Loaded: %s --> %s", tag, tag_path);
    }

    return 0;
}

int write_tag_file(struct list *tags, char *path)
{
    struct node *tag_node;
    struct tag *tag_data;

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        LOG_ERR("Unable to open file at %s", path);
        return 1;
    }

    tag_node = tags->head;
    while (tag_node != NULL) {
        tag_data = (struct tag *)tag_node->data;
        fprintf(f, "%s=%s\n", tag_data->tag, tag_data->path);
        tag_node = tag_node->next;
    }
    fprintf(f, "\n");
    fclose(f);

    LOG_INF("Tag file written to %s", path);
    return 0;
}
