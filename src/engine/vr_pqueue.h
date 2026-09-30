/*
 * vr_pqueue.h - priority queue as a binary max-heap.
 *
 * Items are ordered by `key` (larger leaves first); equal keys fall back to
 * `tiebreak` (larger leaves first). Two users:
 *   - triage:   key = 6 - severity, tiebreak = -arrival_seq
 *               (ESI level 1 leaves first, then earliest arrival)
 *   - Dijkstra: key = -distance, tiebreak = -node_id
 *               (the nearest unsettled vertex leaves first)
 */
#ifndef VR_PQUEUE_H
#define VR_PQUEUE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double key;
    double tiebreak;
    long   value;      /* caller's payload: arrival sequence, vertex id, ... */
} vr_pq_item;

typedef struct {
    vr_pq_item *items; /* items[0] is the maximum */
    size_t      size;
    size_t      capacity;
} vr_pqueue;

/* All int-returning functions return 0 on success, -1 on failure/empty. */
int    vr_pq_init(vr_pqueue *pq, size_t initial_capacity);
void   vr_pq_free(vr_pqueue *pq);
int    vr_pq_insert(vr_pqueue *pq, vr_pq_item item);
int    vr_pq_extract_max(vr_pqueue *pq, vr_pq_item *out);
int    vr_pq_peek(const vr_pqueue *pq, vr_pq_item *out);
int    vr_pq_is_empty(const vr_pqueue *pq);
size_t vr_pq_size(const vr_pqueue *pq);

/* Replace the contents with `n` items and build the heap bottom-up in O(n). */
int    vr_pq_heapify(vr_pqueue *pq, const vr_pq_item *items, size_t n);

/* Remove the first item whose value equals `value` (used by undo). */
int    vr_pq_remove_value(vr_pqueue *pq, long value);

/* 1 if a outranks b under the (key, tiebreak) ordering. */
int    vr_pq_higher(const vr_pq_item *a, const vr_pq_item *b);

#ifdef __cplusplus
}
#endif

#endif /* VR_PQUEUE_H */
