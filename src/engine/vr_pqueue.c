/* vr_pqueue.c - binary max-heap priority queue. */
#include "vr_pqueue.h"

#include <stdlib.h>
#include <string.h>

int vr_pq_higher(const vr_pq_item *a, const vr_pq_item *b)
{
    if (a->key != b->key)
        return a->key > b->key;
    return a->tiebreak > b->tiebreak;
}

static void swap_items(vr_pq_item *a, vr_pq_item *b)
{
    vr_pq_item t = *a;
    *a = *b;
    *b = t;
}

static void sift_up(vr_pqueue *pq, size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (!vr_pq_higher(&pq->items[i], &pq->items[parent]))
            break;
        swap_items(&pq->items[i], &pq->items[parent]);
        i = parent;
    }
}

static void sift_down(vr_pqueue *pq, size_t i)
{
    for (;;) {
        size_t left = 2 * i + 1, right = left + 1, best = i;
        if (left < pq->size && vr_pq_higher(&pq->items[left], &pq->items[best]))
            best = left;
        if (right < pq->size && vr_pq_higher(&pq->items[right], &pq->items[best]))
            best = right;
        if (best == i)
            return;
        swap_items(&pq->items[i], &pq->items[best]);
        i = best;
    }
}

static int reserve(vr_pqueue *pq, size_t needed)
{
    size_t cap;
    vr_pq_item *grown;

    if (needed <= pq->capacity)
        return 0;
    cap = pq->capacity ? pq->capacity : 8;
    while (cap < needed)
        cap *= 2;
    grown = (vr_pq_item *)realloc(pq->items, cap * sizeof *grown);
    if (!grown)
        return -1;
    pq->items = grown;
    pq->capacity = cap;
    return 0;
}

int vr_pq_init(vr_pqueue *pq, size_t initial_capacity)
{
    pq->items = NULL;
    pq->size = 0;
    pq->capacity = 0;
    return reserve(pq, initial_capacity ? initial_capacity : 8);
}

void vr_pq_free(vr_pqueue *pq)
{
    free(pq->items);
    pq->items = NULL;
    pq->size = pq->capacity = 0;
}

int vr_pq_insert(vr_pqueue *pq, vr_pq_item item)
{
    if (reserve(pq, pq->size + 1) != 0)
        return -1;
    pq->items[pq->size] = item;
    sift_up(pq, pq->size);
    pq->size++;
    return 0;
}

int vr_pq_extract_max(vr_pqueue *pq, vr_pq_item *out)
{
    if (pq->size == 0)
        return -1;
    if (out)
        *out = pq->items[0];
    pq->size--;
    if (pq->size > 0) {
        pq->items[0] = pq->items[pq->size];
        sift_down(pq, 0);
    }
    return 0;
}

int vr_pq_peek(const vr_pqueue *pq, vr_pq_item *out)
{
    if (pq->size == 0)
        return -1;
    if (out)
        *out = pq->items[0];
    return 0;
}

int vr_pq_is_empty(const vr_pqueue *pq)
{
    return pq->size == 0;
}

size_t vr_pq_size(const vr_pqueue *pq)
{
    return pq->size;
}

int vr_pq_heapify(vr_pqueue *pq, const vr_pq_item *items, size_t n)
{
    size_t i;

    if (reserve(pq, n) != 0)
        return -1;
    if (n > 0)
        memcpy(pq->items, items, n * sizeof *items);
    pq->size = n;
    /* Floyd's build-heap: sift down every internal node, last to first. */
    for (i = n / 2; i-- > 0;)
        sift_down(pq, i);
    return 0;
}

int vr_pq_remove_value(vr_pqueue *pq, long value)
{
    size_t i;

    for (i = 0; i < pq->size; i++) {
        if (pq->items[i].value != value)
            continue;
        pq->size--;
        if (i < pq->size) {
            pq->items[i] = pq->items[pq->size];
            /* The moved item may need to travel either way. */
            sift_up(pq, i);
            sift_down(pq, i);
        }
        return 0;
    }
    return -1;
}
