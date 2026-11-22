/*
 * vr_hash.h - string-keyed hash table with separate chaining.
 *
 * Indexes patients by identifier ("VR-2026-000123") for O(1) average
 * lookup. The bucket array doubles whenever the load factor would exceed
 * `max_load`, and the table reports how many insertions collided.
 * Keys are copied; values are borrowed pointers owned by the caller.
 */
#ifndef VR_HASH_H
#define VR_HASH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long (*vr_hash_fn)(const char *key);

typedef struct vr_hentry {
    char             *key;
    void             *value;
    struct vr_hentry *next;
} vr_hentry;

typedef struct {
    vr_hentry   **buckets;
    size_t        bucket_count;
    size_t        size;
    size_t        collisions;  /* inserts that landed in a non-empty bucket */
    size_t        resizes;
    double        max_load;
    vr_hash_fn    hash;
} vr_hash;

typedef struct {
    size_t entries;
    size_t buckets;
    size_t used_buckets;
    size_t longest_chain;
    size_t collisions;
    size_t resizes;
    double load_factor;
} vr_hash_stats;

typedef void (*vr_hash_visit)(const char *key, void *value, void *ctx);

unsigned long vr_hash_fnv1a(const char *key);

/* 0 on success, -1 on allocation failure. Default hash is FNV-1a. */
int    vr_hash_init(vr_hash *h, size_t initial_buckets);
/* Same, with a caller-supplied hash (tests use this to force collisions). */
int    vr_hash_init_custom(vr_hash *h, size_t initial_buckets, vr_hash_fn fn);
/* Frees keys and nodes; values are left to the caller. */
void   vr_hash_free(vr_hash *h);

/* 1 = inserted, 0 = existing key's value replaced, -1 = allocation failure. */
int    vr_hash_put(vr_hash *h, const char *key, void *value);
void  *vr_hash_get(const vr_hash *h, const char *key);
int    vr_hash_contains(const vr_hash *h, const char *key);
/* Returns the removed value, or NULL when the key is absent. */
void  *vr_hash_remove(vr_hash *h, const char *key);
size_t vr_hash_size(const vr_hash *h);
double vr_hash_load_factor(const vr_hash *h);
void   vr_hash_foreach(const vr_hash *h, vr_hash_visit visit, void *ctx);
void   vr_hash_get_stats(const vr_hash *h, vr_hash_stats *out);

#ifdef __cplusplus
}
#endif

#endif /* VR_HASH_H */
