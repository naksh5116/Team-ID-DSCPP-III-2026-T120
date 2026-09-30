/* vr_deque.c - circular-array deque that doubles when full. */
#include "vr_deque.h"

#include <stdlib.h>
#include <string.h>

static unsigned char *cell(const vr_deque *d, size_t logical)
{
    return d->data + ((d->head + logical) % d->capacity) * d->elem_size;
}

static int grow(vr_deque *d)
{
    size_t new_cap = d->capacity ? d->capacity * 2 : 8;
    unsigned char *fresh = (unsigned char *)malloc(new_cap * d->elem_size);
    size_t i;

    if (!fresh)
        return -1;
    /* Unwrap the ring so the front lands at index 0 of the new buffer. */
    for (i = 0; i < d->size; i++)
        memcpy(fresh + i * d->elem_size, cell(d, i), d->elem_size);
    free(d->data);
    d->data = fresh;
    d->capacity = new_cap;
    d->head = 0;
    return 0;
}

int vr_deque_init(vr_deque *d, size_t elem_size, size_t initial_capacity)
{
    d->elem_size = elem_size;
    d->capacity = initial_capacity ? initial_capacity : 8;
    d->head = d->size = 0;
    d->data = (unsigned char *)malloc(d->capacity * elem_size);
    return d->data ? 0 : -1;
}

void vr_deque_free(vr_deque *d)
{
    free(d->data);
    d->data = NULL;
    d->capacity = d->head = d->size = 0;
}

int vr_deque_push_front(vr_deque *d, const void *elem)
{
    if (d->size == d->capacity && grow(d) != 0)
        return -1;
    d->head = (d->head + d->capacity - 1) % d->capacity;
    memcpy(d->data + d->head * d->elem_size, elem, d->elem_size);
    d->size++;
    return 0;
}

int vr_deque_push_back(vr_deque *d, const void *elem)
{
    if (d->size == d->capacity && grow(d) != 0)
        return -1;
    memcpy(cell(d, d->size), elem, d->elem_size);
    d->size++;
    return 0;
}

int vr_deque_pop_front(vr_deque *d, void *out)
{
    if (d->size == 0)
        return -1;
    if (out)
        memcpy(out, cell(d, 0), d->elem_size);
    d->head = (d->head + 1) % d->capacity;
    d->size--;
    return 0;
}

int vr_deque_pop_back(vr_deque *d, void *out)
{
    if (d->size == 0)
        return -1;
    if (out)
        memcpy(out, cell(d, d->size - 1), d->elem_size);
    d->size--;
    return 0;
}

int vr_deque_front(const vr_deque *d, void *out)
{
    if (d->size == 0)
        return -1;
    memcpy(out, cell(d, 0), d->elem_size);
    return 0;
}

int vr_deque_back(const vr_deque *d, void *out)
{
    if (d->size == 0)
        return -1;
    memcpy(out, cell(d, d->size - 1), d->elem_size);
    return 0;
}

int vr_deque_is_empty(const vr_deque *d)
{
    return d->size == 0;
}

size_t vr_deque_size(const vr_deque *d)
{
    return d->size;
}

const void *vr_deque_at(const vr_deque *d, size_t i)
{
    return i < d->size ? cell(d, i) : NULL;
}
