#ifndef __LIST_H__
#define __LIST_H__

/* doubly linked list implementation from linux kernel */

#define offset_of(type, member) \
    ((unsigned long)(&(((type*)0)->member)))

#define container_of(ptr, type, member) \
    ((type*)((unsigned long)(ptr) - offset_of(type, member)))

#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct list_node {
    struct list_node *prev, *next;
};

static inline void list_init(struct list_node* list)
{
    list->prev = list;
    list->next = list;
}

static inline void list_add_prev(struct list_node* node,
                                 struct list_node* next)
{
    node->next = next;
    node->prev = next->prev;
    next->prev = node;
    node->prev->next = node;
}

static inline void __list_del(struct list_node* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline void list_del(struct list_node* node)
{
    __list_del(node);
    list_init(node);
}

#define list_for_each(p, head) \
    for ((p) = (head)->next; (p) != (head); (p) = (p)->next)

#define list_for_each_safe(p, n, head) \
    for ((p) = (head)->next, (n) = (p)->next; \
         (p) != (head); \
         (p) = (n), (n) = (p)->next)

#endif