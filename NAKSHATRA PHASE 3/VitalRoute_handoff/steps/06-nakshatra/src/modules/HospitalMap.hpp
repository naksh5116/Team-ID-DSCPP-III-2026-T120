// HospitalMap.hpp - the hospital floor plan.
//
// Owns the C engine's weighted graph (vr_graph), loads it from
// data/hospital_graph.txt with an ifstream, and exposes Dijkstra's shortest
// paths to the router and the navigator.
#ifndef VR_HOSPITAL_MAP_HPP
#define VR_HOSPITAL_MAP_HPP

#include "vr_graph.h"

#include <string>
#include <vector>

class HospitalMap {
public:
    HospitalMap();
    ~HospitalMap();
    HospitalMap(const HospitalMap&) = delete;
    HospitalMap& operator=(const HospitalMap&) = delete;

    // Replaces the current plan. Throws std::runtime_error naming the file
    // and line of the first malformed entry.
    void load(const std::string& path);
    void addNode(int id, int floor, double x, double y, NodeType type, const std::string& name);
    void addEdge(int from, int to, double weight, EdgeType type, bool undirected = true);
    void clear();

    bool hasNode(int id) const;
    const vr_node& node(int id) const;              // throws std::out_of_range
    const vr_edge* edge(int from, int to) const;    // nullptr when not adjacent
    int findByName(const std::string& name) const;  // -1 when absent
    int firstOfType(NodeType type) const;           // lowest id of that type, or -1
    int nodeCount() const;
    int edgeCount() const;                          // undirected edges count twice

    // Vertex sequence from -> to (both included). Throws
    // NoRouteFoundException when `to` cannot be reached.
    std::vector<int> shortestPath(int from, int to, double* total = nullptr) const;
    // Dijkstra from one source to every vertex; VR_UNREACHABLE where there
    // is no path. Indexed by node id.
    std::vector<double> distancesFrom(int source) const;

private:
    vr_graph graph_;
};

#endif // VR_HOSPITAL_MAP_HPP
