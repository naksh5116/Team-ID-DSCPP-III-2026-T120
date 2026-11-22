// RouteSteps.hpp - which part of the path each direction step covers.
//
// Route gives the Dijkstra vertex sequence (path) and the Navigator's text
// steps, but not which edges belong to which step. Navigator::describe()
// walks the path strictly left to right, so describing the first k edges
// reproduces the full route's early steps exactly, and its last step before
// "Arrive" is the step that contains edge k. Calling describe() on every
// prefix therefore labels each edge with its step - using the Navigator
// itself, without copying its turn rules.
//
// Steps that cover no edge (the opening "From ...", a turn, "Exit the
// lift ...", "Arrive ...") are tied to the landmark where they happen.
//
// Used by both front ends: the Qt route map and the web route map.
#ifndef VR_ROUTE_STEPS_HPP
#define VR_ROUTE_STEPS_HPP

#include "Navigator.hpp"

#include <vector>

struct RouteSteps {
    // edgeStep[k] = step index of the edge path[k] -> path[k + 1].
    std::vector<int> edgeStep;
    // For every step: the edges it covers, and the vertex that marks it when
    // it covers none (-1 otherwise).
    std::vector<std::vector<int>> stepEdges;
    std::vector<int> stepVertex;

    bool empty() const { return stepEdges.empty(); }
    // Floor to show when a step is selected (-1 when unknown).
    int floorOfStep(const HospitalMap& map, const Route& route, int step) const;
};

RouteSteps mapRouteSteps(const Navigator& navigator, const Route& route);

#endif // VR_ROUTE_STEPS_HPP
