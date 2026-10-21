/* vr_graph.c - adjacency-list graph and Dijkstra over vr_pqueue. */
#include "vr_graph.h"
#include "vr_pqueue.h"

#include <stdlib.h>
#include <string.h>

static const char *const NODE_NAMES[] = {
    "JUNCTION", "CORRIDOR", "LIFT", "STAIRS", "WARD", "ICU", "OT", "ER", "RECEPTION"
};
static const char *const EDGE_NAMES[] = { "CORRIDOR", "LIFT", "STAIRS", "DOOR" };

void vr_graph_init(vr_graph *g)
{
    g->nodes = NULL;
    g->adj = NULL;
    g->present = NULL;
    g->capacity = 0;
    g->node_count = 0;
    g->edge_count = 0;
}

void vr_graph_free(vr_graph *g)
{
    int i;
    for (i = 0; i < g->capacity; i++) {
        vr_edge *e = g->adj[i];
        while (e) {
            vr_edge *next = e->next;
            free(e);
            e = next;
        }
    }
    free(g->nodes);
    free(g->adj);
    free(g->present);
    vr_graph_init(g);
}

/* Grow the id-indexed arrays so that `id` is addressable. */
static int ensure_capacity(vr_graph *g, int id)
{
    int cap, i;
    vr_node *nodes;
    vr_edge **adj;
    unsigned char *present;

    if (id < g->capacity)
        return 0;
    cap = g->capacity ? g->capacity : 16;
    while (cap <= id)
        cap *= 2;

    nodes = (vr_node *)realloc(g->nodes, (size_t)cap * sizeof *nodes);
    if (!nodes)
        return -1;
    g->nodes = nodes;
    adj = (vr_edge **)realloc(g->adj, (size_t)cap * sizeof *adj);
    if (!adj)
        return -1;
    g->adj = adj;
    present = (unsigned char *)realloc(g->present, (size_t)cap);
    if (!present)
        return -1;
    g->present = present;

    for (i = g->capacity; i < cap; i++) {
        g->adj[i] = NULL;
        g->present[i] = 0;
        memset(&g->nodes[i], 0, sizeof g->nodes[i]);
    }
    g->capacity = cap;
    return 0;
}

int vr_graph_add_node(vr_graph *g, const vr_node *node)
{
    if (node->id < 0 || ensure_capacity(g, node->id) != 0)
        return -1;
    if (g->present[node->id])
        return -1;
    g->nodes[node->id] = *node;
    g->nodes[node->id].name[sizeof node->name - 1] = '\0';
    g->present[node->id] = 1;
    g->node_count++;
    return 0;
}

static int push_edge(vr_graph *g, int from, int to, double weight, EdgeType type)
{
    vr_edge *e = (vr_edge *)malloc(sizeof *e);
    if (!e)
        return -1;
    e->to = to;
    e->weight = weight;
    e->type = type;
    e->next = g->adj[from];
    g->adj[from] = e;
    g->edge_count++;
    return 0;
}

int vr_graph_add_edge(vr_graph *g, int from, int to, double weight,
                      EdgeType type, int undirected)
{
    if (!vr_graph_has_node(g, from) || !vr_graph_has_node(g, to) || weight < 0.0)
        return -1;
    if (push_edge(g, from, to, weight, type) != 0)
        return -1;
    if (undirected && from != to && push_edge(g, to, from, weight, type) != 0)
        return -1;
    return 0;
}

int vr_graph_has_node(const vr_graph *g, int id)
{
    return id >= 0 && id < g->capacity && g->present[id];
}

const vr_node *vr_graph_node(const vr_graph *g, int id)
{
    return vr_graph_has_node(g, id) ? &g->nodes[id] : NULL;
}

const vr_edge *vr_graph_edges(const vr_graph *g, int id)
{
    return vr_graph_has_node(g, id) ? g->adj[id] : NULL;
}

const vr_edge *vr_graph_find_edge(const vr_graph *g, int from, int to)
{
    const vr_edge *e;
    const vr_edge *best = NULL;
    for (e = vr_graph_edges(g, from); e; e = e->next)
        if (e->to == to && (!best || e->weight < best->weight))
            best = e;
    return best;
}

int vr_graph_capacity(const vr_graph *g)
{
    return g->capacity;
}

int vr_graph_dijkstra(const vr_graph *g, int src, double *dist, int *prev)
{
    vr_pqueue frontier;
    vr_pq_item item;
    unsigned char *settled;
    int i;

    if (!vr_graph_has_node(g, src))
        return -1;
    settled = (unsigned char *)calloc((size_t)g->capacity, 1);
    if (!settled)
        return -1;
    if (vr_pq_init(&frontier, (size_t)g->node_count + 1) != 0) {
        free(settled);
        return -1;
    }

    for (i = 0; i < g->capacity; i++) {
        dist[i] = VR_UNREACHABLE;
        prev[i] = -1;
    }
    dist[src] = 0.0;
    item.key = 0.0;             /* max-heap on -distance = min-heap on distance */
    item.tiebreak = -(double)src;
    item.value = src;
    if (vr_pq_insert(&frontier, item) != 0)
        goto fail;

    /* Lazy deletion: a vertex may sit in the heap several times; only the
     * first (shortest) extraction settles it. */
    while (vr_pq_extract_max(&frontier, &item) == 0) {
        int u = (int)item.value;
        const vr_edge *e;
        if (settled[u])
            continue;
        settled[u] = 1;
        for (e = g->adj[u]; e; e = e->next) {
            double candidate = dist[u] + e->weight;
            if (settled[e->to])
                continue;
            if (dist[e->to] == VR_UNREACHABLE || candidate < dist[e->to]) {
                vr_pq_item next;
                dist[e->to] = candidate;
                prev[e->to] = u;
                next.key = -candidate;
                next.tiebreak = -(double)e->to;
                next.value = e->to;
                if (vr_pq_insert(&frontier, next) != 0)
                    goto fail;
            }
        }
    }
    vr_pq_free(&frontier);
    free(settled);
    return 0;

fail:
    vr_pq_free(&frontier);
    free(settled);
    return -1;
}

int vr_graph_shortest_path(const vr_graph *g, int src, int dst,
                           int *path, int max_len, double *total)
{
    double *dist;
    int *prev;
    int len = 0, v, i;

    if (!vr_graph_has_node(g, src) || !vr_graph_has_node(g, dst))
        return -1;
    dist = (double *)malloc((size_t)g->capacity * sizeof *dist);
    prev = (int *)malloc((size_t)g->capacity * sizeof *prev);
    if (!dist || !prev || vr_graph_dijkstra(g, src, dist, prev) != 0) {
        free(dist);
        free(prev);
        return -1;
    }
    if (dist[dst] == VR_UNREACHABLE) {
        free(dist);
        free(prev);
        return 0;
    }
    for (v = dst; v != -1; v = prev[v])
        len++;
    if (len > max_len) {
        free(dist);
        free(prev);
        return -1;
    }
    /* Walk back from dst, filling the path from the end. */
    for (v = dst, i = len - 1; v != -1; v = prev[v], i--)
        path[i] = v;
    if (total)
        *total = dist[dst];
    free(dist);
    free(prev);
    return len;
}

const char *vr_node_type_name(NodeType t)
{
    return ((int)t >= 0 && (int)t <= (int)NODE_RECEPTION) ? NODE_NAMES[t] : "?";
}

const char *vr_edge_type_name(EdgeType t)
{
    return ((int)t >= 0 && (int)t <= (int)EDGE_DOOR) ? EDGE_NAMES[t] : "?";
}

int vr_node_type_parse(const char *s, NodeType *out)
{
    int i;
    for (i = 0; i <= (int)NODE_RECEPTION; i++) {
        if (strcmp(s, NODE_NAMES[i]) == 0) {
            *out = (NodeType)i;
            return 0;
        }
    }
    return -1;
}

int vr_edge_type_parse(const char *s, EdgeType *out)
{
    int i;
    for (i = 0; i <= (int)EDGE_DOOR; i++) {
        if (strcmp(s, EDGE_NAMES[i]) == 0) {
            *out = (EdgeType)i;
            return 0;
        }
    }
    return -1;
}
