/*
 * vr_sort.h - merge sort and lower-bound binary search over arrays of
 * fixed-size elements, in the style of qsort/bsearch.
 *
 *   vr_merge_sort  - stable O(n log n); sorts bill listings (by date or by
 *                    amount) and patient listings (by admission date).
 *   vr_lower_bound - first index whose element is not less than `key`;
 *                    finds the next available doctor on the availability
 *                    array, which is kept sorted by next-free time.
 */
#ifndef VR_SORT_H
#define VR_SORT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* <0, 0, >0 like strcmp. */
typedef int (*vr_cmp_fn)(const void *a, const void *b);

/* 0 on success, -1 if the scratch buffer cannot be allocated. */
int    vr_merge_sort(void *base, size_t n, size_t size, vr_cmp_fn cmp);

/* Smallest i in [0, n] with cmp(&base[i], key) >= 0; n if none. */
size_t vr_lower_bound(const void *base, size_t n, size_t size,
                      const void *key, vr_cmp_fn cmp);

#ifdef __cplusplus
}
#endif

#endif /* VR_SORT_H */
