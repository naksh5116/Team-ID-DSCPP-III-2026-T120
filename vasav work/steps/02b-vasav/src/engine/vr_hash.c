/* vr_hash.c - separate-chaining hash table with load-factor resizing. */
#include "vr_hash.h"

#include <stdlib.h>
#include <string.h>

#define VR_HASH_DEFAULT_LOAD 0.75

unsigned long vr_hash_fnv1a(const char *key)
{
    unsigned long h = 2166136261UL;
    while (*key) {
        h ^= (unsigned char)*key++;
        h *= 16777619UL;
        h &= 0xFFFFFFFFUL;   /* same result on 32- and 64-bit longs */
    }
    return h;
}

static char *copy_string(const char *s)
{
    size_t len = strlen(s) + 1;
    char *c = (char *)malloc(len);
    if (c)
        memcpy(c, s, len);
    return c;
}

static size_t bucket_of(const vr_hash *h, const char *key, size_t bucket_count)
{
    return (size_t)(h->hash(key) % bucket_count);
}

int vr_hash_init_custom(vr_hash *h, size_t initial_buckets, vr_hash_fn fn)
{
    h->bucket_count = initial_buckets ? initial_buckets : 16;
    h->buckets = (vr_hentry **)calloc(h->bucket_count, sizeof *h->buckets);
    h->size = h->collisions = h->resizes = 0;
    h->max_load = VR_HASH_DEFAULT_LOAD;
    h->hash = fn ? fn : vr_hash_fnv1a;
    return h->buckets ? 0 : -1;
}

int vr_hash_init(vr_hash *h, size_t initial_buckets)
{
    return vr_hash_init_custom(h, initial_buckets, NULL);
}

void vr_hash_free(vr_hash *h)
{
    size_t i;
    for (i = 0; i < h->bucket_count; i++) {
        vr_hentry *e = h->buckets[i];
        while (e) {
            vr_hentry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(h->buckets);
    h->buckets = NULL;
    h->bucket_count = h->size = 0;
}

/* Rehash every entry into a bucket array twice the size. Collisions are
 * recounted, since they depend on the bucket count. */
static int resize(vr_hash *h)
{
    size_t new_count = h->bucket_count * 2, i;
    vr_hentry **fresh = (vr_hentry **)calloc(new_count, sizeof *fresh);

    if (!fresh)
        return -1;
    h->collisions = 0;
    for (i = 0; i < h->bucket_count; i++) {
        vr_hentry *e = h->buckets[i];
        while (e) {
            vr_hentry *next = e->next;
            size_t b = bucket_of(h, e->key, new_count);
            if (fresh[b])
                h->collisions++;
            e->next = fresh[b];
            fresh[b] = e;
            e = next;
        }
    }
    free(h->buckets);
    h->buckets = fresh;
    h->bucket_count = new_count;
    h->resizes++;
    return 0;
}

int vr_hash_put(vr_hash *h, const char *key, void *value)
{
    size_t b = bucket_of(h, key, h->bucket_count);
    vr_hentry *e;

    for (e = h->buckets[b]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return 0;
        }
    }

    if ((double)(h->size + 1) / (double)h->bucket_count > h->max_load) {
        if (resize(h) != 0)
            return -1;
        b = bucket_of(h, key, h->bucket_count);
    }

    e = (vr_hentry *)malloc(sizeof *e);
    if (!e)
        return -1;
    e->key = copy_string(key);
    if (!e->key) {
        free(e);
        return -1;
    }
    e->value = value;
    if (h->buckets[b])
        h->collisions++;
    e->next = h->buckets[b];
    h->buckets[b] = e;
    h->size++;
    return 1;
}

void *vr_hash_get(const vr_hash *h, const char *key)
{
    const vr_hentry *e = h->buckets[bucket_of(h, key, h->bucket_count)];
    for (; e; e = e->next)
        if (strcmp(e->key, key) == 0)
            return e->value;
    return NULL;
}

int vr_hash_contains(const vr_hash *h, const char *key)
{
    const vr_hentry *e = h->buckets[bucket_of(h, key, h->bucket_count)];
    for (; e; e = e->next)
        if (strcmp(e->key, key) == 0)
            return 1;
    return 0;
}

void *vr_hash_remove(vr_hash *h, const char *key)
{
    size_t b = bucket_of(h, key, h->bucket_count);
    vr_hentry *prev = NULL, *e = h->buckets[b];

    for (; e; prev = e, e = e->next) {
        void *value;
        if (strcmp(e->key, key) != 0)
            continue;
        if (prev)
            prev->next = e->next;
        else
            h->buckets[b] = e->next;
        /* A key that shared its bucket counted as a collision on insert. */
        if ((prev || e->next) && h->collisions > 0)
            h->collisions--;
        value = e->value;
        free(e->key);
        free(e);
        h->size--;
        return value;
    }
    return NULL;
}

size_t vr_hash_size(const vr_hash *h)
{
    return h->size;
}

double vr_hash_load_factor(const vr_hash *h)
{
    return h->bucket_count ? (double)h->size / (double)h->bucket_count : 0.0;
}

void vr_hash_foreach(const vr_hash *h, vr_hash_visit visit, void *ctx)
{
    size_t i;
    for (i = 0; i < h->bucket_count; i++) {
        vr_hentry *e = h->buckets[i];
        while (e) {
            vr_hentry *next = e->next;   /* visit may not touch the table */
            visit(e->key, e->value, ctx);
            e = next;
        }
    }
}

void vr_hash_get_stats(const vr_hash *h, vr_hash_stats *out)
{
    size_t i;

    out->entries = h->size;
    out->buckets = h->bucket_count;
    out->used_buckets = 0;
    out->longest_chain = 0;
    out->collisions = h->collisions;
    out->resizes = h->resizes;
    out->load_factor = vr_hash_load_factor(h);
    for (i = 0; i < h->bucket_count; i++) {
        size_t len = 0;
        const vr_hentry *e;
        for (e = h->buckets[i]; e; e = e->next)
            len++;
        if (len) {
            out->used_buckets++;
            if (len > out->longest_chain)
                out->longest_chain = len;
        }
    }
}
