/*
 * vr_stack.h - growable LIFO stack of fixed-size elements.
 *
 * Elements are copied in by value, so any plain struct can be stacked.
 * The application keeps its undo journal (last reception or billing entry)
 * on one of these.
 */
#ifndef VR_STACK_H
#define VR_STACK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *data;
    size_t elem_size;
    size_t size;
    size_t capacity;
} vr_stack;

/* int-returning functions: 0 on success, -1 on failure/empty. */
int    vr_stack_init(vr_stack *s, size_t elem_size, size_t initial_capacity);
void   vr_stack_free(vr_stack *s);
int    vr_stack_push(vr_stack *s, const void *elem);   /* doubles when full */
int    vr_stack_pop(vr_stack *s, void *out);
int    vr_stack_peek(const vr_stack *s, void *out);
int    vr_stack_is_empty(const vr_stack *s);
size_t vr_stack_size(const vr_stack *s);
size_t vr_stack_capacity(const vr_stack *s);
void   vr_stack_clear(vr_stack *s);

#ifdef __cplusplus
}
#endif

#endif /* VR_STACK_H */
