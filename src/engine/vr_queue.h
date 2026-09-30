/*
 * vr_queue.h - FIFO queue on a singly linked list with head and tail
 * pointers, so enqueue and dequeue are both O(1).
 *
 * Holds the OPD (outpatient) line in arrival order.
 */
#ifndef VR_QUEUE_H
#define VR_QUEUE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vr_qnode vr_qnode;   /* opaque */

typedef struct {
    vr_qnode *head;
    vr_qnode *tail;
    size_t    size;
    size_t    elem_size;
} vr_queue;

typedef int (*vr_queue_pred)(const void *elem, void *ctx);

/* int-returning functions: 0 on success, -1 on failure/empty. */
void   vr_queue_init(vr_queue *q, size_t elem_size);
void   vr_queue_free(vr_queue *q);
int    vr_queue_enqueue(vr_queue *q, const void *elem);
int    vr_queue_dequeue(vr_queue *q, void *out);
int    vr_queue_front(const vr_queue *q, void *out);
int    vr_queue_is_empty(const vr_queue *q);
size_t vr_queue_size(const vr_queue *q);
/* i-th element from the front (O(i)), or NULL. */
const void *vr_queue_at(const vr_queue *q, size_t i);
/* Unlink the first element matching `pred` (used by undo); -1 if none. */
int    vr_queue_remove_first(vr_queue *q, vr_queue_pred pred, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VR_QUEUE_H */
