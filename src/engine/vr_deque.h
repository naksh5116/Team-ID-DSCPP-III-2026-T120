/*
 * vr_deque.h - double-ended queue on a growable circular array.
 *
 * The transfer desk pushes urgent requests at the front, routine ones at the
 * back, always serves from the front, and can withdraw the newest routine
 * request from the back.
 */
#ifndef VR_DEQUE_H
#define VR_DEQUE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *data;
    size_t elem_size;
    size_t capacity;
    size_t head;   /* physical index of the front element */
    size_t size;
} vr_deque;

/* int-returning functions: 0 on success, -1 on failure/empty. */
int    vr_deque_init(vr_deque *d, size_t elem_size, size_t initial_capacity);
void   vr_deque_free(vr_deque *d);
int    vr_deque_push_front(vr_deque *d, const void *elem);
int    vr_deque_push_back(vr_deque *d, const void *elem);
int    vr_deque_pop_front(vr_deque *d, void *out);
int    vr_deque_pop_back(vr_deque *d, void *out);
int    vr_deque_front(const vr_deque *d, void *out);
int    vr_deque_back(const vr_deque *d, void *out);
int    vr_deque_is_empty(const vr_deque *d);
size_t vr_deque_size(const vr_deque *d);
/* i-th element counting from the front, or NULL. */
const void *vr_deque_at(const vr_deque *d, size_t i);

#ifdef __cplusplus
}
#endif

#endif /* VR_DEQUE_H */
