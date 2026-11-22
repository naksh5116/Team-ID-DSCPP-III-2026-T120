// RouteSteps.cpp
#include "RouteSteps.hpp"

RouteSteps mapRouteSteps(const Navigator& navigator, const Route& route)
{
    RouteSteps seg;
    const int steps = static_cast<int>(route.steps.size());
    if (steps == 0)
        return seg;
    seg.stepEdges.resize(static_cast<std::size_t>(steps));
    seg.stepVertex.assign(static_cast<std::size_t>(steps), -1);

    const std::vector<int>& path = route.path;
    const int edges = path.size() > 1 ? static_cast<int>(path.size()) - 1 : 0;
    for (int k = 0; k < edges; ++k) {
        const std::vector<int> prefix(path.begin(), path.begin() + k + 2);
        const Route partial = navigator.describe(prefix);
        // The last step before "Arrive at ..." is the one that holds edge k.
        int step = static_cast<int>(partial.steps.size()) - 2;
        if (step < 0)
            step = 0;
        if (step >= steps - 1)
            step = steps - 2;   // never the Arrive step
        seg.edgeStep.push_back(step);
        if (step >= 0)
            seg.stepEdges[static_cast<std::size_t>(step)].push_back(k);
    }

    // Steps without edges happen at a landmark: the start of the next
    // step's first edge (or the destination for "Arrive").
    for (int s = 0; s < steps; ++s) {
        if (!seg.stepEdges[static_cast<std::size_t>(s)].empty())
            continue;
        if (s == steps - 1) {
            seg.stepVertex[static_cast<std::size_t>(s)] = path.empty() ? -1 : path.back();
            continue;
        }
        int vertex = path.empty() ? -1 : path.front();
        for (int t = s + 1; t < steps; ++t)
            if (!seg.stepEdges[static_cast<std::size_t>(t)].empty()) {
                vertex = path.at(static_cast<std::size_t>(seg.stepEdges[static_cast<std::size_t>(t)].front()));
                break;
            }
        seg.stepVertex[static_cast<std::size_t>(s)] = vertex;
    }
    return seg;
}

int RouteSteps::floorOfStep(const HospitalMap& map, const Route& route, int step) const
{
    if (step < 0 || step >= static_cast<int>(stepEdges.size()) || route.path.empty())
        return -1;
    const std::vector<int>& edges = stepEdges[static_cast<std::size_t>(step)];
    if (!edges.empty()) {
        // For a lift or stairs step, show the floor it arrives on.
        return map.node(route.path.at(static_cast<std::size_t>(edges.back()) + 1)).floor;
    }
    const int v = stepVertex[static_cast<std::size_t>(step)];
    return v >= 0 ? map.node(v).floor : -1;
}
