/* vr_stack.c - array-backed stack with geometric growth. */
#include "vr_stack.h"

#include <stdlib.h>
#include <string.h>

int vr_stack_init(vr_stack *s, size_t elem_size, size_t initial_capacity)
{
    s->elem_size = elem_size;
    s->size = 0;
    s->capacity = initial_capacity ? initial_capacity : 4;
    s->data = (unsigned char *)malloc(s->capacity * elem_size);
    return s->data ? 0 : -1;
}

void vr_stack_free(vr_stack *s)
{
    free(s->data);
    s->data = NULL;
    s->size = s->capacity = 0;
}

int vr_stack_push(vr_stack *s, const void *elem)
{
    if (s->size == s->capacity) {
        size_t new_cap = s->capacity ? s->capacity * 2 : 4;
        unsigned char *grown = (unsigned char *)realloc(s->data, new_cap * s->elem_size);
        if (!grown)
            return -1;
        s->data = grown;
        s->capacity = new_cap;
    }
    memcpy(s->data + s->size * s->elem_size, elem, s->elem_size);
    s->size++;
    return 0;
}

int vr_stack_pop(vr_stack *s, void *out)
{
    if (s->size == 0)
        return -1;
    s->size--;
    if (out)
        memcpy(out, s->data + s->size * s->elem_size, s->elem_size);
    return 0;
}

int vr_stack_peek(const vr_stack *s, void *out)
{
    if (s->size == 0)
        return -1;
    if (out)
        memcpy(out, s->data + (s->size - 1) * s->elem_size, s->elem_size);
    return 0;
}

int vr_stack_is_empty(const vr_stack *s)
{
    return s->size == 0;
}

size_t vr_stack_size(const vr_stack *s)
{
    return s->size;
}

size_t vr_stack_capacity(const vr_stack *s)
{
    return s->capacity;
}

void vr_stack_clear(vr_stack *s)
{
    s->size = 0;
}
