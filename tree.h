
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>

#include "list.h"
#include <unistd.h> 
#include <stddef.h>

#define container_of(ptr, type, member) \
    ((type*)((unsigned long)(ptr) - offset_of(type, member)))
struct rb_node {
        unsigned long  __rb_parent_color;
        struct rb_node *rb_right;
        struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */
 
struct rb_root {
        struct rb_node *rb_node;
};

struct memfs_file {
    char *path;                     /* File path */
    void *data;                     /* File content */
 
    struct stat vstat;              /* File stat */
 
    pthread_mutex_t lock;
    pthread_cond_t write_finished;
 
    struct rb_node node;
};
static struct memfs_file *__search(struct rb_root *root, const char *path)
{
    struct memfs_file *pf = NULL;
    struct rb_node *node = root->rb_node;
 
    while (node) {
        pf = container_of(node, struct memfs_file, node);
 
        int result = strcmp(path, pf->path);
        if (result < 0) {
            node = node->rb_left;
        }
        else if (result > 0) {
            node = node->rb_right;
        }
        else {
            return pf;
        }
    }
 
//  printf("%s not found!!!\n", path);
    return NULL;
}
static struct memfs_file *__search(struct rb_root *root, const char *path)
{
    struct memfs_file *pf = NULL;
    struct rb_node *node = root->rb_node;
 
    while (node) {
        pf = container_of(node, struct memfs_file, node);
 
        int result = strcmp(path, pf->path);
        if (result < 0) {
            node = node->rb_left;
        }
        else if (result > 0) {
            node = node->rb_right;
        }
        else {
            return pf;
        }
    }
 
//  printf("%s not found!!!\n", path);
    return NULL;
}