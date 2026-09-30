/* vr_sort.c - top-down merge sort and lower-bound binary search. */
#include "vr_sort.h"

#include <stdlib.h>
#include <string.h>

#define AT(p, i, size) ((unsigned char *)(p) + (size_t)(i) * (size))

static void merge_range(unsigned char *base, unsigned char *tmp, size_t lo,
                        size_t hi, size_t size, vr_cmp_fn cmp)
{
    size_t mid, i, j, k;

    if (hi - lo < 2)
        return;
    mid = lo + (hi - lo) / 2;
    merge_range(base, tmp, lo, mid, size, cmp);
    merge_range(base, tmp, mid, hi, size, cmp);

    /* Already ordered across the split: nothing to merge. */
    if (cmp(AT(base, mid - 1, size), AT(base, mid, size)) <= 0)
        return;

    i = lo;
    j = mid;
    k = lo;
    while (i < mid && j < hi) {
        /* `<=` takes from the left run on ties, which keeps the sort stable. */
        if (cmp(AT(base, i, size), AT(base, j, size)) <= 0)
            memcpy(AT(tmp, k++, size), AT(base, i++, size), size);
        else
            memcpy(AT(tmp, k++, size), AT(base, j++, size), size);
    }
    while (i < mid)
        memcpy(AT(tmp, k++, size), AT(base, i++, size), size);
    while (j < hi)
        memcpy(AT(tmp, k++, size), AT(base, j++, size), size);
    memcpy(AT(base, lo, size), AT(tmp, lo, size), (hi - lo) * size);
}

int vr_merge_sort(void *base, size_t n, size_t size, vr_cmp_fn cmp)
{
    unsigned char *tmp;

    if (n < 2)
        return 0;
    tmp = (unsigned char *)malloc(n * size);
    if (!tmp)
        return -1;
    merge_range((unsigned char *)base, tmp, 0, n, size, cmp);
    free(tmp);
    return 0;
}

size_t vr_lower_bound(const void *base, size_t n, size_t size,
                      const void *key, vr_cmp_fn cmp)
{
    size_t lo = 0, hi = n;   /* answer lies in [lo, hi] */

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (cmp(AT(base, mid, size), key) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}
