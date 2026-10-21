// HospitalMap.cpp
#include "HospitalMap.hpp"

#include "Exceptions.hpp"

#include <cstdio>
#include <fstream>
#include <new>
#include <sstream>

HospitalMap::HospitalMap()
{
    vr_graph_init(&graph_);
}

HospitalMap::~HospitalMap()
{
    vr_graph_free(&graph_);
}

void HospitalMap::clear()
{
    vr_graph_free(&graph_);
}

void HospitalMap::addNode(int id, int floor, double x, double y, NodeType type,
                          const std::string& name)
{
    vr_node n{};
    n.id = id;
    n.floor = floor;
    n.x = x;
    n.y = y;
    n.type = type;
    std::snprintf(n.name, sizeof n.name, "%s", name.c_str());
    if (vr_graph_add_node(&graph_, &n) != 0)
        throw std::invalid_argument("node " + std::to_string(id) + " is negative or duplicated");
}

void HospitalMap::addEdge(int from, int to, double weight, EdgeType type, bool undirected)
{
    if (vr_graph_add_edge(&graph_, from, to, weight, type, undirected ? 1 : 0) != 0)
        throw std::invalid_argument("edge " + std::to_string(from) + "-" + std::to_string(to) +
                                    " has an unknown endpoint or a negative weight");
}

void HospitalMap::load(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open floor plan " + path);
    clear();

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream fields(line);
        std::string tag;
        if (!(fields >> tag) || tag[0] == '#')
            continue;
        try {
            if (tag == "N") {
                int id = 0, floor = 0;
                double x = 0, y = 0;
                std::string typeName, name;
                NodeType type = NODE_JUNCTION;
                if (!(fields >> id >> floor >> x >> y >> typeName))
                    throw std::invalid_argument("expected: N id floor x y type name");
                if (vr_node_type_parse(typeName.c_str(), &type) != 0)
                    throw std::invalid_argument("unknown node type " + typeName);
                std::getline(fields >> std::ws, name);
                while (!name.empty() && (name.back() == '\r' || name.back() == ' '))
                    name.pop_back();
                if (name.empty())
                    throw std::invalid_argument("node needs a name");
                addNode(id, floor, x, y, type, name);
            } else if (tag == "E") {
                int from = 0, to = 0;
                double weight = 0;
                std::string typeName, flag;
                EdgeType type = EDGE_CORRIDOR;
                if (!(fields >> from >> to >> weight >> typeName))
                    throw std::invalid_argument("expected: E from to weight type [D]");
                if (vr_edge_type_parse(typeName.c_str(), &type) != 0)
                    throw std::invalid_argument("unknown edge type " + typeName);
                fields >> flag;
                addEdge(from, to, weight, type, flag != "D");
            } else {
                throw std::invalid_argument("unknown record '" + tag + "'");
            }
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
    if (graph_.node_count == 0)
        throw std::runtime_error(path + ": no nodes found");
}

bool HospitalMap::hasNode(int id) const
{
    return vr_graph_has_node(&graph_, id) != 0;
}

const vr_node& HospitalMap::node(int id) const
{
    const vr_node* n = vr_graph_node(&graph_, id);
    if (!n)
        throw std::out_of_range("no node " + std::to_string(id) + " on the floor plan");
    return *n;
}

const vr_edge* HospitalMap::edge(int from, int to) const
{
    return vr_graph_find_edge(&graph_, from, to);
}

int HospitalMap::findByName(const std::string& name) const
{
    for (int id = 0; id < vr_graph_capacity(&graph_); ++id)
        if (hasNode(id) && name == graph_.nodes[id].name)
            return id;
    return -1;
}

int HospitalMap::firstOfType(NodeType type) const
{
    for (int id = 0; id < vr_graph_capacity(&graph_); ++id)
        if (hasNode(id) && graph_.nodes[id].type == type)
            return id;
    return -1;
}

int HospitalMap::nodeCount() const
{
    return graph_.node_count;
}

int HospitalMap::edgeCount() const
{
    return graph_.edge_count;
}

std::vector<int> HospitalMap::shortestPath(int from, int to, double* total) const
{
    if (!hasNode(from) || !hasNode(to))
        throw NoRouteFoundException("route requested between unknown locations " +
                                    std::to_string(from) + " and " + std::to_string(to));
    std::vector<int> path(static_cast<std::size_t>(graph_.node_count));
    const int len = vr_graph_shortest_path(&graph_, from, to, path.data(),
                                           static_cast<int>(path.size()), total);
    if (len < 0)
        throw std::bad_alloc();
    if (len == 0)
        throw NoRouteFoundException(std::string("no route from ") + node(from).name + " to " +
                                    node(to).name);
    path.resize(static_cast<std::size_t>(len));
    return path;
}

std::vector<double> HospitalMap::distancesFrom(int source) const
{
    const std::size_t cap = static_cast<std::size_t>(vr_graph_capacity(&graph_));
    std::vector<double> dist(cap, VR_UNREACHABLE);
    std::vector<int> prev(cap, -1);
    if (!hasNode(source))
        throw NoRouteFoundException("unknown route origin " + std::to_string(source));
    if (vr_graph_dijkstra(&graph_, source, dist.data(), prev.data()) != 0)
        throw std::bad_alloc();
    return dist;
}
