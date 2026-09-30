/* vr_cqueue.c - circular queue with an explicit element count. */
#include "vr_cqueue.h"

#include <stdlib.h>
#include <string.h>

int vr_cq_init(vr_cqueue *q, size_t capacity, size_t elem_size)
{
    q->data = NULL;
    q->elem_size = elem_size;
    q->capacity = capacity;
    q->head = q->tail = q->count = 0;
    if (capacity == 0 || elem_size == 0)
        return -1;
    q->data = (unsigned char *)malloc(capacity * elem_size);
    return q->data ? 0 : -1;
}

void vr_cq_free(vr_cqueue *q)
{
    free(q->data);
    q->data = NULL;
    q->capacity = q->head = q->tail = q->count = 0;
}

int vr_cq_enqueue(vr_cqueue *q, const void *elem)
{
    if (q->count == q->capacity)
        return -1;
    memcpy(q->data + q->tail * q->elem_size, elem, q->elem_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    return 0;
}

int vr_cq_dequeue(vr_cqueue *q, void *out)
{
    if (q->count == 0)
        return -1;
    if (out)
        memcpy(out, q->data + q->head * q->elem_size, q->elem_size);
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    return 0;
}

int vr_cq_front(const vr_cqueue *q, void *out)
{
    if (q->count == 0)
        return -1;
    if (out)
        memcpy(out, q->data + q->head * q->elem_size, q->elem_size);
    return 0;
}

int vr_cq_is_empty(const vr_cqueue *q)
{
    return q->count == 0;
}

int vr_cq_is_full(const vr_cqueue *q)
{
    return q->count == q->capacity;
}

size_t vr_cq_size(const vr_cqueue *q)
{
    return q->count;
}

void vr_cq_clear(vr_cqueue *q)
{
    q->head = q->tail = q->count = 0;
}

const void *vr_cq_at(const vr_cqueue *q, size_t i)
{
    if (i >= q->count)
        return NULL;
    return q->data + ((q->head + i) % q->capacity) * q->elem_size;
}

const void *vr_cq_cell(const vr_cqueue *q, size_t physical_index)
{
    if (physical_index >= q->capacity)
        return NULL;
    return q->data + physical_index * q->elem_size;
}
