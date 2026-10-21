/*
 * vr_dlist.h - doubly linked list of fixed-size elements.
 *
 * Each patient owns one of these for their visit history. Insertion at the
 * head or tail is O(1); traversal works in both directions (the lookup
 * screen prints newest-first by walking backwards from the tail).
 */
#ifndef VR_DLIST_H
#define VR_DLIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vr_dnode vr_dnode;   /* opaque */

typedef struct {
    vr_dnode *head;
    vr_dnode *tail;
    size_t    size;
    size_t    elem_size;
} vr_dlist;

void   vr_dlist_init(vr_dlist *l, size_t elem_size);
void   vr_dlist_free(vr_dlist *l);                      /* frees every node */

/* Insert a copy of *elem; return the new node or NULL on allocation failure. */
vr_dnode *vr_dlist_insert_head(vr_dlist *l, const void *elem);
vr_dnode *vr_dlist_insert_tail(vr_dlist *l, const void *elem);
vr_dnode *vr_dlist_insert_after(vr_dlist *l, vr_dnode *node, const void *elem);

/* Unlink and free a node; copies its element to *out first when out != NULL. */
void   vr_dlist_remove(vr_dlist *l, vr_dnode *node, void *out);
size_t vr_dlist_size(const vr_dlist *l);

/* Traversal. */
vr_dnode   *vr_dlist_head(const vr_dlist *l);
vr_dnode   *vr_dlist_tail(const vr_dlist *l);
vr_dnode   *vr_dnode_next(const vr_dnode *n);
vr_dnode   *vr_dnode_prev(const vr_dnode *n);
void       *vr_dnode_data(vr_dnode *n);
const void *vr_dnode_cdata(const vr_dnode *n);

#ifdef __cplusplus
}
#endif

#endif /* VR_DLIST_H */
