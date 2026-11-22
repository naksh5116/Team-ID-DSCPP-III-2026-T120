// RouteSegments.hpp - which part of the path each direction step covers,
// in Qt types for the route map widgets.
//
// The algorithm lives in src/app/RouteSteps (shared with the web version):
// Navigator::describe() is run on every prefix of the path, so each edge is
// labelled with its step by the Navigator itself. Steps that cover no edge
// (the opening "From ...", a turn, "Exit the lift ...", "Arrive ...") are
// tied to the landmark where they happen.
#ifndef VR_GUI_ROUTE_SEGMENTS_HPP
#define VR_GUI_ROUTE_SEGMENTS_HPP

#include "Navigator.hpp"

#include <QList>

struct RouteSegments {
    // edgeStep[k] = step index of the edge path[k] -> path[k + 1].
    QList<int> edgeStep;
    // For every step: the edges (indices into edgeStep) it covers, and the
    // vertex that marks it when it covers none (-1 otherwise).
    QList<QList<int>> stepEdges;
    QList<int> stepVertex;

    bool isEmpty() const { return stepEdges.isEmpty(); }
    // Floor shown when a step is selected.
    int floorOfStep(const HospitalMap& map, const Route& route, int step) const;
};

RouteSegments mapRouteSegments(const Navigator& navigator, const Route& route);

#endif // VR_GUI_ROUTE_SEGMENTS_HPP
