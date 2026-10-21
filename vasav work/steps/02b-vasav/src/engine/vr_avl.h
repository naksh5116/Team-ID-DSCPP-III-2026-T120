/*
 * vr_avl.h - AVL tree keyed on admission date (YYYYMMDD as an int).
 *
 * Several patients can share a date, so each node keeps a small array of
 * values. Balance is restored with the four classic rotations (LL, RR, LR,
 * RL); the tree counts how often each one fires. vr_avl_range walks
 * in-order over [lo, hi], skipping subtrees that cannot overlap the range.
 */
#ifndef VR_AVL_H
#define VR_AVL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vr_avl_node {
    int                 key;
    void              **values;
    size_t              count;
    size_t              cap;
    int                 height;    /* leaf = 1 */
    struct vr_avl_node *left;
    struct vr_avl_node *right;
} vr_avl_node;

typedef struct {
    vr_avl_node  *root;
    size_t        nodes;     /* distinct keys  */
    size_t        values;    /* stored values  */
    unsigned long rot_ll;    /* single right rotation (left-left case)   */
    unsigned long rot_rr;    /* single left rotation (right-right case)  */
    unsigned long rot_lr;    /* double rotation (left-right case)        */
    unsigned long rot_rl;    /* double rotation (right-left case)        */
} vr_avl;

typedef void (*vr_avl_visit)(int key, void *value, void *ctx);

void   vr_avl_init(vr_avl *t);
void   vr_avl_free(vr_avl *t);               /* values are not freed */

/* 0 on success, -1 on allocation failure. */
int    vr_avl_insert(vr_avl *t, int key, void *value);
/* Remove one (key, value) pair; the node goes when its last value does.
 * 0 on success, -1 if the pair is absent. */
int    vr_avl_remove(vr_avl *t, int key, void *value);

/* Visits every value with lo <= key <= hi in ascending key order and
 * returns how many were visited. `visit` may be NULL to just count. */
size_t vr_avl_range(const vr_avl *t, int lo, int hi, vr_avl_visit visit, void *ctx);

int    vr_avl_height(const vr_avl *t);
int    vr_avl_balance_factor(const vr_avl_node *n);   /* height(L) - height(R) */
/* 1 if BST order, stored heights and |balance| <= 1 all hold everywhere. */
int    vr_avl_validate(const vr_avl *t);

#ifdef __cplusplus
}
#endif

#endif /* VR_AVL_H */
