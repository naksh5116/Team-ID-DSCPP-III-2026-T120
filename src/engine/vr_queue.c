/* vr_queue.c - linked-list FIFO queue. */
#include "vr_queue.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct vr_qnode {
    struct vr_qnode *next;
    max_align_t      payload[];   /* elem_size bytes, suitably aligned */
};

void vr_queue_init(vr_queue *q, size_t elem_size)
{
    q->head = q->tail = NULL;
    q->size = 0;
    q->elem_size = elem_size;
}

void vr_queue_free(vr_queue *q)
{
    vr_qnode *n = q->head;
    while (n) {
        vr_qnode *next = n->next;
        free(n);
        n = next;
    }
    q->head = q->tail = NULL;
    q->size = 0;
}

int vr_queue_enqueue(vr_queue *q, const void *elem)
{
    vr_qnode *n = (vr_qnode *)malloc(sizeof *n + q->elem_size);
    if (!n)
        return -1;
    n->next = NULL;
    memcpy(n->payload, elem, q->elem_size);
    if (q->tail)
        q->tail->next = n;
    else
        q->head = n;
    q->tail = n;
    q->size++;
    return 0;
}

int vr_queue_dequeue(vr_queue *q, void *out)
{
    vr_qnode *n = q->head;
    if (!n)
        return -1;
    if (out)
        memcpy(out, n->payload, q->elem_size);
    q->head = n->next;
    if (!q->head)
        q->tail = NULL;
    free(n);
    q->size--;
    return 0;
}

int vr_queue_front(const vr_queue *q, void *out)
{
    if (!q->head)
        return -1;
    if (out)
        memcpy(out, q->head->payload, q->elem_size);
    return 0;
}

int vr_queue_is_empty(const vr_queue *q)
{
    return q->size == 0;
}

size_t vr_queue_size(const vr_queue *q)
{
    return q->size;
}

const void *vr_queue_at(const vr_queue *q, size_t i)
{
    vr_qnode *n = q->head;
    while (n && i--)
        n = n->next;
    return n ? (const void *)n->payload : NULL;
}

int vr_queue_remove_first(vr_queue *q, vr_queue_pred pred, void *ctx)
{
    vr_qnode *prev = NULL, *n = q->head;

    while (n && !pred(n->payload, ctx)) {
        prev = n;
        n = n->next;
    }
    if (!n)
        return -1;
    if (prev)
        prev->next = n->next;
    else
        q->head = n->next;
    if (q->tail == n)
        q->tail = prev;
    free(n);
    q->size--;
    return 0;
}
