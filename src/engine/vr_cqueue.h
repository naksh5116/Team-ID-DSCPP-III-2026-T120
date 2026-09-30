/*
 * vr_cqueue.h - fixed-capacity circular queue (ring buffer).
 *
 * head and tail wrap with modulo arithmetic. Because head == tail both when
 * the ring is empty and when it is full, an explicit `count` field tells the
 * two states apart.
 *
 * The operation theatre scheduler keeps one ring per theatre holding the
 * indices of that day's free slots: booking dequeues the next free slot in
 * cycle order, releasing enqueues it again - O(1), nothing else moves.
 */
#ifndef VR_CQUEUE_H
#define VR_CQUEUE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *data;
    size_t elem_size;
    size_t capacity;
    size_t head;   /* physical index of the front element  */
    size_t tail;   /* physical index where the next enqueue lands */
    size_t count;  /* number of stored elements            */
} vr_cqueue;

/* int-returning functions: 0 on success, -1 on failure (full/empty/alloc). */
int    vr_cq_init(vr_cqueue *q, size_t capacity, size_t elem_size);
void   vr_cq_free(vr_cqueue *q);
int    vr_cq_enqueue(vr_cqueue *q, const void *elem);
int    vr_cq_dequeue(vr_cqueue *q, void *out);
int    vr_cq_front(const vr_cqueue *q, void *out);
int    vr_cq_is_empty(const vr_cqueue *q);
int    vr_cq_is_full(const vr_cqueue *q);
size_t vr_cq_size(const vr_cqueue *q);
void   vr_cq_clear(vr_cqueue *q);

/* i-th element counting from the front (0 = front), or NULL. */
const void *vr_cq_at(const vr_cqueue *q, size_t i);
/* Raw storage cell by physical index, or NULL (used by tests and displays). */
const void *vr_cq_cell(const vr_cqueue *q, size_t physical_index);

#ifdef __cplusplus
}
#endif

#endif /* VR_CQUEUE_H */
