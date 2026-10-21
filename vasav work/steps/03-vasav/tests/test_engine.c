/*
 * test_engine.c - unit tests for the pure-C engine (src/engine).
 *
 * Required tests T1-T4 run first, then extra coverage (X1-X5) for the rest
 * of the engine. Every test prints one PASS/FAIL line; failed checks are
 * listed underneath. The exit status is the number of failed tests.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "vr_avl.h"
#include "vr_cqueue.h"
#include "vr_deque.h"
#include "vr_dlist.h"
#include "vr_hash.h"
#include "vr_pqueue.h"
#include "vr_queue.h"
#include "vr_sort.h"
#include "vr_stack.h"

static int g_failed_checks;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            g_failed_checks++;                                               \
            printf("       check failed: %s  (%s:%d)\n", #cond, __FILE__,   \
                   __LINE__);                                                \
        }                                                                    \
    } while (0)

static int run(const char *label, void (*fn)(void))
{
    int before = g_failed_checks;
    fn();
    printf("[%s] %s\n", g_failed_checks == before ? "PASS" : "FAIL", label);
    return g_failed_checks == before;
}

/* Small deterministic generator so runs are repeatable. */
static unsigned long g_seed = 12345UL;
static unsigned long next_rand(void)
{
    g_seed = (g_seed * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return g_seed;
}

/* ------------------------------------------------------------------ T1 -- */

static vr_pq_item triage_item(int severity, int arrival_seq)
{
    vr_pq_item it;
    it.key = 6.0 - severity;        /* ESI 1 is most critical */
    it.tiebreak = -(double)arrival_seq;
    it.value = arrival_seq;
    return it;
}

static void test_priority_queue(void)
{
    /* (severity, arrival) inserted out of order */
    static const int arrivals[][2] = {
        {3, 1}, {1, 2}, {4, 3}, {2, 4}, {1, 5}, {5, 6}, {3, 7}, {2, 8}
    };
    static const long expected[] = {2, 5, 4, 8, 1, 7, 3, 6};
    const size_t n = sizeof arrivals / sizeof arrivals[0];
    vr_pq_item items[8], it, prev;
    vr_pqueue pq;
    size_t i;

    CHECK(vr_pq_init(&pq, 2) == 0);   /* small start forces growth */
    CHECK(vr_pq_is_empty(&pq));
    CHECK(vr_pq_extract_max(&pq, &it) == -1);
    for (i = 0; i < n; i++) {
        items[i] = triage_item(arrivals[i][0], arrivals[i][1]);
        CHECK(vr_pq_insert(&pq, items[i]) == 0);
    }
    CHECK(vr_pq_size(&pq) == n);
    CHECK(vr_pq_peek(&pq, &it) == 0 && it.value == 2);
    for (i = 0; i < n; i++) {
        CHECK(vr_pq_extract_max(&pq, &it) == 0);
        CHECK(it.value == expected[i]);
    }
    CHECK(vr_pq_is_empty(&pq));

    /* heapify (bottom-up build) must give the same order */
    CHECK(vr_pq_heapify(&pq, items, n) == 0);
    for (i = 0; i < n; i++) {
        CHECK(vr_pq_extract_max(&pq, &it) == 0);
        CHECK(it.value == expected[i]);
    }

    /* removing a waiting patient keeps the heap valid */
    CHECK(vr_pq_heapify(&pq, items, n) == 0);
    CHECK(vr_pq_remove_value(&pq, 4) == 0);
    CHECK(vr_pq_remove_value(&pq, 99) == -1);
    {
        static const long without4[] = {2, 5, 8, 1, 7, 3, 6};
        for (i = 0; i < 7; i++) {
            CHECK(vr_pq_extract_max(&pq, &it) == 0);
            CHECK(it.value == without4[i]);
        }
    }

    /* randomised: output never increases in (key, tiebreak) order */
    for (i = 0; i < 500; i++)
        CHECK(vr_pq_insert(&pq, triage_item((int)(next_rand() % 5) + 1, (int)i)) == 0);
    CHECK(vr_pq_extract_max(&pq, &prev) == 0);
    while (vr_pq_extract_max(&pq, &it) == 0) {
        CHECK(!vr_pq_higher(&it, &prev));
        prev = it;
    }
    vr_pq_free(&pq);
}

/* ------------------------------------------------------------------ T2 -- */

static void test_circular_queue(void)
{
    vr_cqueue ring, day;
    int i, v, owner[6];

    /* Raw ring: full and empty both have head == tail; count decides. */
    CHECK(vr_cq_init(&ring, 5, sizeof(int)) == 0);
    CHECK(vr_cq_is_empty(&ring) && !vr_cq_is_full(&ring));
    CHECK(ring.head == ring.tail);
    for (i = 0; i < 5; i++)
        CHECK(vr_cq_enqueue(&ring, &i) == 0);
    CHECK(vr_cq_is_full(&ring) && !vr_cq_is_empty(&ring));
    CHECK(ring.head == ring.tail);            /* same indices as when empty */
    CHECK(vr_cq_enqueue(&ring, &i) == -1);    /* full rejects */

    /* Wrap-around: the tail re-enters physical cells 0 and 1. */
    CHECK(vr_cq_dequeue(&ring, &v) == 0 && v == 0);
    CHECK(vr_cq_dequeue(&ring, &v) == 0 && v == 1);
    v = 5;
    CHECK(vr_cq_enqueue(&ring, &v) == 0);
    v = 6;
    CHECK(vr_cq_enqueue(&ring, &v) == 0);
    CHECK(ring.tail == 2 && ring.head == 2);
    CHECK(*(const int *)vr_cq_cell(&ring, 0) == 5);
    CHECK(*(const int *)vr_cq_cell(&ring, 1) == 6);
    for (i = 2; i < 5; i++)                   /* untouched: nothing shifted */
        CHECK(*(const int *)vr_cq_cell(&ring, (size_t)i) == i);
    for (i = 0; i < 5; i++)
        CHECK(*(const int *)vr_cq_at(&ring, (size_t)i) == i + 2);
    while (vr_cq_dequeue(&ring, NULL) == 0)
        ;
    CHECK(vr_cq_is_empty(&ring) && !vr_cq_is_full(&ring) && ring.head == ring.tail);
    CHECK(vr_cq_front(&ring, &v) == -1);
    vr_cq_free(&ring);

    /* Theatre day: six slots, the ring holds the free slot numbers. */
    CHECK(vr_cq_init(&day, 6, sizeof(int)) == 0);
    for (i = 0; i < 6; i++) {
        CHECK(vr_cq_enqueue(&day, &i) == 0);
        owner[i] = 0;
    }
    for (i = 0; i < 6; i++) {                 /* fill the day */
        CHECK(vr_cq_dequeue(&day, &v) == 0 && v == i);
        owner[v] = 100 + i;
    }
    CHECK(vr_cq_is_empty(&day) && day.head == day.tail);   /* fully booked */
    CHECK(vr_cq_dequeue(&day, &v) == -1);

    v = 3;                                    /* release a middle slot */
    owner[3] = 0;
    CHECK(vr_cq_enqueue(&day, &v) == 0);
    CHECK(vr_cq_size(&day) == 1);
    CHECK(vr_cq_dequeue(&day, &v) == 0 && v == 3);        /* reused at once */
    owner[v] = 200;
    for (i = 0; i < 6; i++)                   /* neighbours never moved */
        CHECK(owner[i] == (i == 3 ? 200 : 100 + i));
    CHECK(vr_cq_is_empty(&day));
    vr_cq_free(&day);
}

/* ------------------------------------------------------------------ T3 -- */

static int is_leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

/* YYYYMMDD for `offset` days after 1 Jan 2024. */
static int date_after(int offset)
{
    static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int y = 2024, m = 0, d = 1;
    while (offset-- > 0) {
        int len = mdays[m] + (m == 1 && is_leap(y));
        if (++d > len) {
            d = 1;
            if (++m == 12) {
                m = 0;
                y++;
            }
        }
    }
    return y * 10000 + (m + 1) * 100 + d;
}

typedef struct {
    int last;
    int ordered;
    int count;
} range_ctx;

static void range_visit(int key, void *value, void *ctx)
{
    range_ctx *r = (range_ctx *)ctx;
    (void)value;
    if (key < r->last)
        r->ordered = 0;
    r->last = key;
    r->count++;
}

static void rotation_case(int a, int b, int c, int want_root, const char *name)
{
    vr_avl t;
    int dummy = 0;
    vr_avl_init(&t);
    CHECK(vr_avl_insert(&t, a, &dummy) == 0);
    CHECK(vr_avl_insert(&t, b, &dummy) == 0);
    CHECK(vr_avl_insert(&t, c, &dummy) == 0);
    CHECK(t.root && t.root->key == want_root);
    CHECK(vr_avl_height(&t) == 2);
    /* exactly the expected rotation fired, and nothing else */
    CHECK(t.rot_ll + t.rot_rr + t.rot_lr + t.rot_rl == 1);
    if (strcmp(name, "LL") == 0) CHECK(t.rot_ll == 1);
    if (strcmp(name, "RR") == 0) CHECK(t.rot_rr == 1);
    if (strcmp(name, "LR") == 0) CHECK(t.rot_lr == 1);
    if (strcmp(name, "RL") == 0) CHECK(t.rot_rl == 1);
    vr_avl_free(&t);
}

static void test_avl(void)
{
    enum { N = 1000 };
    static int patient[N];
    vr_avl t;
    range_ctx r;
    int i;
    double bound = 1.44 * log2((double)N);

    rotation_case(30, 20, 10, 20, "LL");
    rotation_case(10, 20, 30, 20, "RR");
    rotation_case(30, 10, 20, 20, "LR");
    rotation_case(10, 30, 20, 20, "RL");

    /* 1000 ascending admission dates: the worst case for a plain BST. */
    vr_avl_init(&t);
    for (i = 0; i < N; i++) {
        patient[i] = i;
        CHECK(vr_avl_insert(&t, date_after(i), &patient[i]) == 0);
    }
    CHECK(t.nodes == N && t.values == N);
    CHECK(vr_avl_validate(&t));
    CHECK((double)vr_avl_height(&t) <= bound);
    CHECK(t.rot_rr > 0);                      /* ascending input rotates left */

    /* Range query: February 2024 has 29 days. */
    r.last = 0; r.ordered = 1; r.count = 0;
    CHECK(vr_avl_range(&t, 20240201, 20240229, range_visit, &r) == 29);
    CHECK(r.count == 29 && r.ordered);
    CHECK(vr_avl_range(&t, 20240301, 20240201, NULL, NULL) == 0);    /* lo > hi */
    CHECK(vr_avl_range(&t, 0, 99999999, NULL, NULL) == N);

    /* Several patients on one date share a node. */
    {
        int extra = -1;
        CHECK(vr_avl_insert(&t, 20240215, &extra) == 0);
        CHECK(t.nodes == N && t.values == N + 1);
        CHECK(vr_avl_range(&t, 20240215, 20240215, NULL, NULL) == 2);
        CHECK(vr_avl_remove(&t, 20240215, &extra) == 0);
        CHECK(vr_avl_range(&t, 20240215, 20240215, NULL, NULL) == 1);
        CHECK(vr_avl_remove(&t, 20240215, &extra) == -1);
    }

    /* Deleting every other date keeps the tree balanced. */
    for (i = 0; i < N; i += 2)
        CHECK(vr_avl_remove(&t, date_after(i), &patient[i]) == 0);
    CHECK(t.nodes == N / 2);
    CHECK(vr_avl_validate(&t));
    CHECK((double)vr_avl_height(&t) <= 1.44 * log2((double)(N / 2)) + 1.0);
    vr_avl_free(&t);
    CHECK(t.root == NULL);
}

/* ------------------------------------------------------------------ T4 -- */

static unsigned long same_bucket(const char *key)
{
    (void)key;
    return 42UL;    /* every key collides */
}

static void test_hash(void)
{
    enum { N = 60 };
    static int values[N];
    char key[32];
    vr_hash h;
    vr_hash_stats st;
    int i;

    CHECK(vr_hash_init_custom(&h, 4, same_bucket) == 0);
    for (i = 0; i < N; i++) {
        values[i] = i;
        sprintf(key, "VR-2026-%06d", i + 1);
        CHECK(vr_hash_put(&h, key, &values[i]) == 1);
    }
    vr_hash_get_stats(&h, &st);
    CHECK(st.entries == N);
    CHECK(st.used_buckets == 1 && st.longest_chain == N);
    CHECK(st.collisions == N - 1);
    CHECK(st.resizes > 0);                    /* load factor forced growth */
    CHECK(st.load_factor <= 0.75);
    for (i = 0; i < N; i++) {                 /* all still retrievable */
        sprintf(key, "VR-2026-%06d", i + 1);
        CHECK(vr_hash_get(&h, key) == &values[i]);
    }
    CHECK(vr_hash_get(&h, "VR-2026-999999") == NULL);

    /* replace, then remove from the head, middle and tail of the chain */
    CHECK(vr_hash_put(&h, "VR-2026-000001", &values[5]) == 0);
    CHECK(vr_hash_get(&h, "VR-2026-000001") == &values[5]);
    CHECK(vr_hash_remove(&h, "VR-2026-000060") == &values[59]);
    CHECK(vr_hash_remove(&h, "VR-2026-000030") == &values[29]);
    CHECK(vr_hash_remove(&h, "VR-2026-000001") == &values[5]);
    CHECK(vr_hash_remove(&h, "VR-2026-000001") == NULL);
    CHECK(vr_hash_size(&h) == N - 3);
    CHECK(!vr_hash_contains(&h, "VR-2026-000030"));
    CHECK(vr_hash_get(&h, "VR-2026-000031") == &values[30]);
    vr_hash_get_stats(&h, &st);
    CHECK(st.collisions == N - 4);
    vr_hash_free(&h);

    /* default FNV-1a hash: spreads keys and keeps every one findable */
    CHECK(vr_hash_init(&h, 8) == 0);
    for (i = 0; i < N; i++) {
        sprintf(key, "VR-2026-%06d", i + 1);
        CHECK(vr_hash_put(&h, key, &values[i]) == 1);
    }
    vr_hash_get_stats(&h, &st);
    CHECK(st.used_buckets > 1 && st.load_factor <= 0.75);
    for (i = 0; i < N; i++) {
        sprintf(key, "VR-2026-%06d", i + 1);
        CHECK(vr_hash_get(&h, key) == &values[i]);
    }
    vr_hash_free(&h);
}

/* ------------------------------------------------------------ extras -- */

static void test_stack(void)
{
    vr_stack s;
    int i, v;

    CHECK(vr_stack_init(&s, sizeof(int), 2) == 0);
    CHECK(vr_stack_is_empty(&s) && vr_stack_pop(&s, &v) == -1);
    for (i = 0; i < 100; i++)
        CHECK(vr_stack_push(&s, &i) == 0);
    CHECK(vr_stack_size(&s) == 100 && vr_stack_capacity(&s) >= 100);
    CHECK(vr_stack_peek(&s, &v) == 0 && v == 99);
    for (i = 99; i >= 0; i--)
        CHECK(vr_stack_pop(&s, &v) == 0 && v == i);
    CHECK(vr_stack_is_empty(&s) && vr_stack_peek(&s, &v) == -1);
    vr_stack_free(&s);
}

static int equals_int(const void *elem, void *ctx)
{
    return *(const int *)elem == *(const int *)ctx;
}

static void test_queue(void)
{
    vr_queue q;
    int i, v, target;

    vr_queue_init(&q, sizeof(int));
    CHECK(vr_queue_is_empty(&q) && vr_queue_dequeue(&q, &v) == -1);
    for (i = 1; i <= 5; i++)
        CHECK(vr_queue_enqueue(&q, &i) == 0);
    CHECK(vr_queue_front(&q, &v) == 0 && v == 1);
    CHECK(*(const int *)vr_queue_at(&q, 4) == 5 && vr_queue_at(&q, 5) == NULL);
    target = 3;
    CHECK(vr_queue_remove_first(&q, equals_int, &target) == 0);
    target = 5;                               /* removing the tail */
    CHECK(vr_queue_remove_first(&q, equals_int, &target) == 0);
    v = 6;
    CHECK(vr_queue_enqueue(&q, &v) == 0);     /* tail pointer still valid */
    {
        static const int order[] = {1, 2, 4, 6};
        for (i = 0; i < 4; i++)
            CHECK(vr_queue_dequeue(&q, &v) == 0 && v == order[i]);
    }
    CHECK(vr_queue_is_empty(&q) && q.tail == NULL);
    vr_queue_free(&q);
}

static void test_deque(void)
{
    vr_deque d;
    int i, v;

    CHECK(vr_deque_init(&d, sizeof(int), 4) == 0);
    for (i = 1; i <= 3; i++)
        CHECK(vr_deque_push_back(&d, &i) == 0);
    v = 0;
    CHECK(vr_deque_push_front(&d, &v) == 0);
    v = -1;
    CHECK(vr_deque_push_front(&d, &v) == 0);  /* forces growth while wrapped */
    for (i = 0; i < 5; i++)
        CHECK(*(const int *)vr_deque_at(&d, (size_t)i) == i - 1);
    CHECK(vr_deque_front(&d, &v) == 0 && v == -1);
    CHECK(vr_deque_back(&d, &v) == 0 && v == 3);
    CHECK(vr_deque_pop_back(&d, &v) == 0 && v == 3);
    CHECK(vr_deque_pop_front(&d, &v) == 0 && v == -1);
    CHECK(vr_deque_size(&d) == 3);
    while (vr_deque_pop_front(&d, NULL) == 0)
        ;
    CHECK(vr_deque_is_empty(&d) && vr_deque_pop_back(&d, &v) == -1);
    vr_deque_free(&d);
}

static void test_dlist(void)
{
    vr_dlist l;
    vr_dnode *n, *two;
    int v, i;
    static const int forward[] = {1, 2, 25, 3};

    vr_dlist_init(&l, sizeof(int));
    v = 2;
    two = vr_dlist_insert_tail(&l, &v);
    v = 3;
    CHECK(vr_dlist_insert_tail(&l, &v) != NULL);
    v = 1;
    CHECK(vr_dlist_insert_head(&l, &v) != NULL);
    v = 25;
    CHECK(vr_dlist_insert_after(&l, two, &v) != NULL);
    CHECK(vr_dlist_size(&l) == 4);

    for (i = 0, n = vr_dlist_head(&l); n; n = vr_dnode_next(n), i++)
        CHECK(*(const int *)vr_dnode_cdata(n) == forward[i]);
    CHECK(i == 4);
    for (i = 3, n = vr_dlist_tail(&l); n; n = vr_dnode_prev(n), i--)
        CHECK(*(const int *)vr_dnode_cdata(n) == forward[i]);
    CHECK(i == -1);

    vr_dlist_remove(&l, vr_dlist_head(&l), &v);
    CHECK(v == 1);
    vr_dlist_remove(&l, vr_dlist_tail(&l), &v);
    CHECK(v == 3);
    CHECK(*(const int *)vr_dnode_cdata(vr_dlist_head(&l)) == 2);
    CHECK(*(const int *)vr_dnode_cdata(vr_dlist_tail(&l)) == 25);
    CHECK(vr_dnode_prev(vr_dlist_head(&l)) == NULL);
    vr_dlist_free(&l);
    CHECK(vr_dlist_size(&l) == 0 && vr_dlist_head(&l) == NULL);
}

typedef struct {
    int key;
    int order;
} keyed;

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int cmp_keyed(const void *a, const void *b)
{
    return cmp_int(&((const keyed *)a)->key, &((const keyed *)b)->key);
}

static void test_sort_and_search(void)
{
    enum { N = 1000 };
    static int values[N];
    static keyed recs[N];
    static const int sorted[] = {1, 3, 3, 3, 7, 9};
    int i, key;

    for (i = 0; i < N; i++)
        values[i] = (int)(next_rand() % 500);
    CHECK(vr_merge_sort(values, N, sizeof(int), cmp_int) == 0);
    for (i = 1; i < N; i++)
        CHECK(values[i - 1] <= values[i]);

    for (i = 0; i < N; i++) {                 /* stability: ties keep input order */
        recs[i].key = (int)(next_rand() % 10);
        recs[i].order = i;
    }
    CHECK(vr_merge_sort(recs, N, sizeof(keyed), cmp_keyed) == 0);
    for (i = 1; i < N; i++) {
        CHECK(recs[i - 1].key <= recs[i].key);
        if (recs[i - 1].key == recs[i].key)
            CHECK(recs[i - 1].order < recs[i].order);
    }

    key = 3;  CHECK(vr_lower_bound(sorted, 6, sizeof(int), &key, cmp_int) == 1);
    key = 4;  CHECK(vr_lower_bound(sorted, 6, sizeof(int), &key, cmp_int) == 4);
    key = 0;  CHECK(vr_lower_bound(sorted, 6, sizeof(int), &key, cmp_int) == 0);
    key = 10; CHECK(vr_lower_bound(sorted, 6, sizeof(int), &key, cmp_int) == 6);
    CHECK(vr_lower_bound(sorted, 0, sizeof(int), &key, cmp_int) == 0);
    for (key = -1; key < 501; key++) {        /* agrees with a linear scan */
        size_t lin = 0;
        while (lin < N && values[lin] < key)
            lin++;
        CHECK(vr_lower_bound(values, N, sizeof(int), &key, cmp_int) == lin);
    }
}

int main(void)
{
    int failed = 0;

    printf("VitalRoute engine tests (C)\n");
    failed += !run("T1  Priority queue ordering (severity, then arrival)", test_priority_queue);
    failed += !run("T2  Circular queue wrap-around and slot reuse", test_circular_queue);
    failed += !run("T3  AVL balance on 1000 sorted dates, all four rotations", test_avl);
    failed += !run("T4  Hash lookup with forced collisions", test_hash);
    failed += !run("X1  Stack push/pop with dynamic growth", test_stack);
    failed += !run("X2  FIFO queue order and removal", test_queue);
    failed += !run("X3  Deque operations at both ends", test_deque);
    failed += !run("X4  Doubly linked list, forward and backward", test_dlist);
    failed += !run("X5  Merge sort stability and lower-bound search", test_sort_and_search);
    printf("engine: %d test(s) failed\n", failed);
    return failed;
}
