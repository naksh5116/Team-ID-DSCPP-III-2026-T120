/* vr_avl.c - self-balancing AVL tree with duplicate-key buckets. */
#include "vr_avl.h"

#include <limits.h>
#include <stdlib.h>

static int height(const vr_avl_node *n)
{
    return n ? n->height : 0;
}

static int max_int(int a, int b)
{
    return a > b ? a : b;
}

static void update_height(vr_avl_node *n)
{
    n->height = 1 + max_int(height(n->left), height(n->right));
}

int vr_avl_balance_factor(const vr_avl_node *n)
{
    return n ? height(n->left) - height(n->right) : 0;
}

/*      y            x
 *     / \          / \
 *    x   C  ==>   A   y
 *   / \              / \
 *  A   B            B   C      */
static vr_avl_node *rotate_right(vr_avl_node *y)
{
    vr_avl_node *x = y->left;
    y->left = x->right;
    x->right = y;
    update_height(y);
    update_height(x);
    return x;
}

/* Mirror image of rotate_right. */
static vr_avl_node *rotate_left(vr_avl_node *x)
{
    vr_avl_node *y = x->right;
    x->right = y->left;
    y->left = x;
    update_height(x);
    update_height(y);
    return y;
}

/* Restore the AVL property at n after an insert or delete below it. */
static vr_avl_node *rebalance(vr_avl *t, vr_avl_node *n)
{
    int bf;

    update_height(n);
    bf = vr_avl_balance_factor(n);
    if (bf > 1) {
        if (vr_avl_balance_factor(n->left) >= 0) {
            t->rot_ll++;                    /* LL: one right rotation */
        } else {
            t->rot_lr++;                    /* LR: left on child, right on n */
            n->left = rotate_left(n->left);
        }
        return rotate_right(n);
    }
    if (bf < -1) {
        if (vr_avl_balance_factor(n->right) <= 0) {
            t->rot_rr++;                    /* RR: one left rotation */
        } else {
            t->rot_rl++;                    /* RL: right on child, left on n */
            n->right = rotate_right(n->right);
        }
        return rotate_left(n);
    }
    return n;
}

static int add_value(vr_avl_node *n, void *value)
{
    if (n->count == n->cap) {
        size_t cap = n->cap ? n->cap * 2 : 2;
        void **grown = (void **)realloc(n->values, cap * sizeof *grown);
        if (!grown)
            return -1;
        n->values = grown;
        n->cap = cap;
    }
    n->values[n->count++] = value;
    return 0;
}

static vr_avl_node *new_node(int key, void *value)
{
    vr_avl_node *n = (vr_avl_node *)calloc(1, sizeof *n);
    if (!n)
        return NULL;
    n->key = key;
    n->height = 1;
    if (add_value(n, value) != 0) {
        free(n);
        return NULL;
    }
    return n;
}

static vr_avl_node *insert_rec(vr_avl *t, vr_avl_node *n, int key, void *value, int *status)
{
    if (!n) {
        vr_avl_node *fresh = new_node(key, value);
        if (!fresh) {
            *status = -1;
            return NULL;
        }
        t->nodes++;
        t->values++;
        return fresh;
    }
    if (key < n->key) {
        vr_avl_node *child = insert_rec(t, n->left, key, value, status);
        if (*status != 0)
            return n;
        n->left = child;
    } else if (key > n->key) {
        vr_avl_node *child = insert_rec(t, n->right, key, value, status);
        if (*status != 0)
            return n;
        n->right = child;
    } else {
        if (add_value(n, value) != 0)
            *status = -1;
        else
            t->values++;
        return n;   /* shape unchanged */
    }
    return rebalance(t, n);
}

int vr_avl_insert(vr_avl *t, int key, void *value)
{
    int status = 0;
    vr_avl_node *root = insert_rec(t, t->root, key, value, &status);
    if (status == 0)
        t->root = root;
    return status;
}

static void free_node(vr_avl_node *n)
{
    free(n->values);
    free(n);
}

/* Detach the minimum node of subtree n into *min_out; return the new root. */
static vr_avl_node *detach_min(vr_avl *t, vr_avl_node *n, vr_avl_node **min_out)
{
    if (!n->left) {
        *min_out = n;
        return n->right;
    }
    n->left = detach_min(t, n->left, min_out);
    return rebalance(t, n);
}

static vr_avl_node *remove_rec(vr_avl *t, vr_avl_node *n, int key, void *value, int *found)
{
    if (!n)
        return NULL;
    if (key < n->key) {
        n->left = remove_rec(t, n->left, key, value, found);
    } else if (key > n->key) {
        n->right = remove_rec(t, n->right, key, value, found);
    } else {
        size_t i;
        for (i = 0; i < n->count && n->values[i] != value; i++)
            ;
        if (i == n->count)
            return n;                        /* value not stored here */
        *found = 1;
        n->values[i] = n->values[--n->count];
        t->values--;
        if (n->count > 0)
            return n;                        /* node still in use */

        t->nodes--;
        if (!n->left || !n->right) {
            vr_avl_node *child = n->left ? n->left : n->right;
            free_node(n);
            return child;
        } else {
            /* Two children: splice in the in-order successor. */
            vr_avl_node *succ = NULL;
            vr_avl_node *right = detach_min(t, n->right, &succ);
            succ->right = right;
            succ->left = n->left;
            free_node(n);
            n = succ;
        }
    }
    return rebalance(t, n);
}

int vr_avl_remove(vr_avl *t, int key, void *value)
{
    int found = 0;
    t->root = remove_rec(t, t->root, key, value, &found);
    return found ? 0 : -1;
}

static void free_rec(vr_avl_node *n)
{
    if (!n)
        return;
    free_rec(n->left);
    free_rec(n->right);
    free_node(n);
}

void vr_avl_init(vr_avl *t)
{
    t->root = NULL;
    t->nodes = t->values = 0;
    t->rot_ll = t->rot_rr = t->rot_lr = t->rot_rl = 0;
}

void vr_avl_free(vr_avl *t)
{
    free_rec(t->root);
    vr_avl_init(t);
}

static size_t range_rec(const vr_avl_node *n, int lo, int hi, vr_avl_visit visit, void *ctx)
{
    size_t visited = 0, i;

    if (!n)
        return 0;
    if (lo < n->key)                  /* left subtree may still hold keys >= lo */
        visited += range_rec(n->left, lo, hi, visit, ctx);
    if (lo <= n->key && n->key <= hi) {
        for (i = 0; i < n->count; i++)
            if (visit)
                visit(n->key, n->values[i], ctx);
        visited += n->count;
    }
    if (n->key < hi)                  /* right subtree may still hold keys <= hi */
        visited += range_rec(n->right, lo, hi, visit, ctx);
    return visited;
}

size_t vr_avl_range(const vr_avl *t, int lo, int hi, vr_avl_visit visit, void *ctx)
{
    if (lo > hi)
        return 0;
    return range_rec(t->root, lo, hi, visit, ctx);
}

int vr_avl_height(const vr_avl *t)
{
    return height(t->root);
}

/* Returns the subtree height, or -1 if any invariant is broken. */
static int validate_rec(const vr_avl_node *n, long long lo, long long hi)
{
    int hl, hr;

    if (!n)
        return 0;
    if ((long long)n->key <= lo || (long long)n->key >= hi || n->count == 0)
        return -1;
    hl = validate_rec(n->left, lo, n->key);
    hr = validate_rec(n->right, n->key, hi);
    if (hl < 0 || hr < 0 || hl - hr > 1 || hr - hl > 1)
        return -1;
    if (n->height != 1 + max_int(hl, hr))
        return -1;
    return n->height;
}

int vr_avl_validate(const vr_avl *t)
{
    return validate_rec(t->root, (long long)INT_MIN - 1, (long long)INT_MAX + 1) >= 0;
}
