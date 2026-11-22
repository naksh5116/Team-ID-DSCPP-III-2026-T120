// RouteSegments.cpp - Qt-typed view of the shared step mapping
// (src/app/RouteSteps.cpp holds the algorithm; the web map uses it too).
#include "RouteSegments.hpp"

#include "RouteSteps.hpp"

RouteSegments mapRouteSegments(const Navigator& navigator, const Route& route)
{
    const RouteSteps steps = mapRouteSteps(navigator, route);
    RouteSegments seg;
    for (int s : steps.edgeStep)
        seg.edgeStep.append(s);
    for (const std::vector<int>& edges : steps.stepEdges) {
        QList<int> list;
        for (int k : edges)
            list.append(k);
        seg.stepEdges.append(list);
    }
    for (int v : steps.stepVertex)
        seg.stepVertex.append(v);
    return seg;
}

int RouteSegments::floorOfStep(const HospitalMap& map, const Route& route, int step) const
{
    if (step < 0 || step >= stepEdges.size() || route.path.empty())
        return -1;
    if (!stepEdges.at(step).isEmpty()) {
        // For a lift or stairs step, show the floor it arrives on.
        const int lastEdge = stepEdges.at(step).last();
        return map.node(route.path.at(static_cast<std::size_t>(lastEdge) + 1)).floor;
    }
    const int v = stepVertex.at(step);
    return v >= 0 ? map.node(v).floor : -1;
}
