/* vr_dlist.c - doubly linked list. */
#include "vr_dlist.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct vr_dnode {
    struct vr_dnode *prev;
    struct vr_dnode *next;
    max_align_t      payload[];
};

static vr_dnode *make_node(const vr_dlist *l, const void *elem)
{
    vr_dnode *n = (vr_dnode *)malloc(sizeof *n + l->elem_size);
    if (!n)
        return NULL;
    n->prev = n->next = NULL;
    memcpy(n->payload, elem, l->elem_size);
    return n;
}

void vr_dlist_init(vr_dlist *l, size_t elem_size)
{
    l->head = l->tail = NULL;
    l->size = 0;
    l->elem_size = elem_size;
}

void vr_dlist_free(vr_dlist *l)
{
    vr_dnode *n = l->head;
    while (n) {
        vr_dnode *next = n->next;
        free(n);
        n = next;
    }
    l->head = l->tail = NULL;
    l->size = 0;
}

vr_dnode *vr_dlist_insert_head(vr_dlist *l, const void *elem)
{
    vr_dnode *n = make_node(l, elem);
    if (!n)
        return NULL;
    n->next = l->head;
    if (l->head)
        l->head->prev = n;
    else
        l->tail = n;
    l->head = n;
    l->size++;
    return n;
}

vr_dnode *vr_dlist_insert_tail(vr_dlist *l, const void *elem)
{
    vr_dnode *n = make_node(l, elem);
    if (!n)
        return NULL;
    n->prev = l->tail;
    if (l->tail)
        l->tail->next = n;
    else
        l->head = n;
    l->tail = n;
    l->size++;
    return n;
}

vr_dnode *vr_dlist_insert_after(vr_dlist *l, vr_dnode *node, const void *elem)
{
    vr_dnode *n;

    if (!node || node == l->tail)
        return node ? vr_dlist_insert_tail(l, elem) : vr_dlist_insert_head(l, elem);
    n = make_node(l, elem);
    if (!n)
        return NULL;
    n->prev = node;
    n->next = node->next;
    node->next->prev = n;
    node->next = n;
    l->size++;
    return n;
}

void vr_dlist_remove(vr_dlist *l, vr_dnode *node, void *out)
{
    if (!node)
        return;
    if (out)
        memcpy(out, node->payload, l->elem_size);
    if (node->prev)
        node->prev->next = node->next;
    else
        l->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        l->tail = node->prev;
    free(node);
    l->size--;
}

size_t vr_dlist_size(const vr_dlist *l)
{
    return l->size;
}

vr_dnode *vr_dlist_head(const vr_dlist *l)
{
    return l->head;
}

vr_dnode *vr_dlist_tail(const vr_dlist *l)
{
    return l->tail;
}

vr_dnode *vr_dnode_next(const vr_dnode *n)
{
    return n ? n->next : NULL;
}

vr_dnode *vr_dnode_prev(const vr_dnode *n)
{
    return n ? n->prev : NULL;
}

void *vr_dnode_data(vr_dnode *n)
{
    return n ? (void *)n->payload : NULL;
}

const void *vr_dnode_cdata(const vr_dnode *n)
{
    return n ? (const void *)n->payload : NULL;
}
