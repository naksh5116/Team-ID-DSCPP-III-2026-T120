/*
 * vr_graph.h - weighted hospital floor-plan graph (adjacency lists) and
 * Dijkstra's shortest paths.
 *
 * Vertices are landmarks with a floor and (x, y) coordinates in metres
 * (y grows "north"). Edges carry a weight - metres for corridors, stairs and
 * doors, a time-equivalent for lifts - and a type the navigator uses to
 * phrase floor changes. Dijkstra's frontier is the binary max-heap from
 * vr_pqueue, keyed on negated distance.
 */
#ifndef VR_GRAPH_H
#define VR_GRAPH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { NODE_JUNCTION, NODE_CORRIDOR, NODE_LIFT, NODE_STAIRS,
               NODE_WARD, NODE_ICU, NODE_OT, NODE_ER, NODE_RECEPTION } NodeType;

typedef struct {
    int      id;
    char     name[64];      /* human landmark, e.g. "Nursing Station 2B" */
    int      floor;         /* 0 = ground */
    double   x, y;          /* metres on the floor plan, y increases "north" */
    NodeType type;
} vr_node;

typedef enum { EDGE_CORRIDOR, EDGE_LIFT, EDGE_STAIRS, EDGE_DOOR } EdgeType;

typedef struct vr_edge {
    int             to;
    double          weight;   /* metres for corridors; time-equivalent for lifts */
    EdgeType        type;
    struct vr_edge *next;
} vr_edge;

typedef struct {
    vr_node        *nodes;     /* indexed by id                        */
    vr_edge       **adj;       /* adjacency list head per id           */
    unsigned char  *present;   /* 1 where an id has been added         */
    int             capacity;  /* ids 0 .. capacity-1 are addressable  */
    int             node_count;
    int             edge_count;/* directed adjacency entries           */
} vr_graph;

#define VR_UNREACHABLE (-1.0)

void vr_graph_init(vr_graph *g);
void vr_graph_free(vr_graph *g);

/* 0 on success; -1 on a negative/duplicate id or allocation failure. */
int  vr_graph_add_node(vr_graph *g, const vr_node *node);
/* Adds from->to (and to->from when undirected != 0). -1 on unknown
 * endpoints, a negative weight, or allocation failure. */
int  vr_graph_add_edge(vr_graph *g, int from, int to, double weight,
                       EdgeType type, int undirected);

int             vr_graph_has_node(const vr_graph *g, int id);
const vr_node  *vr_graph_node(const vr_graph *g, int id);        /* NULL if absent */
const vr_edge  *vr_graph_edges(const vr_graph *g, int id);       /* adjacency list */
const vr_edge  *vr_graph_find_edge(const vr_graph *g, int from, int to);
int             vr_graph_capacity(const vr_graph *g);

/* Single-source shortest paths. `dist` and `prev` must hold
 * vr_graph_capacity(g) entries. Unreachable vertices get
 * dist = VR_UNREACHABLE and prev = -1. Returns 0, or -1 on a bad source
 * or allocation failure. */
int  vr_graph_dijkstra(const vr_graph *g, int src, double *dist, int *prev);

/* Shortest path src -> dst written to path[0..len-1] (src first).
 * Returns the vertex count, 0 if dst is unreachable, -1 on error or when
 * max_len is too small. *total receives the path weight when non-NULL. */
int  vr_graph_shortest_path(const vr_graph *g, int src, int dst,
                            int *path, int max_len, double *total);

/* Text <-> enum helpers for the data files ("ICU", "LIFT", ...).
 * Parsers return 0 on success, -1 on an unknown name. */
const char *vr_node_type_name(NodeType t);
const char *vr_edge_type_name(EdgeType t);
int         vr_node_type_parse(const char *s, NodeType *out);
int         vr_edge_type_parse(const char *s, EdgeType *out);

#ifdef __cplusplus
}
#endif

#endif /* VR_GRAPH_H */
