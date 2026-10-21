// Navigator.hpp - turns a shortest path into directions a person can follow.
//
//  1. Dijkstra (C engine, heap frontier) gives the vertex sequence.
//  2. Each same-floor segment A->B gets a bearing atan2(dy, dx) in degrees.
//  3. At every B of A->B->C the change of bearing, folded into (-180, 180],
//     is classified: <20 straight, 20-60 bear slightly, 60-135 turn,
//     >135 turn sharply; positive = left, negative = right.
//  4. Lift and stair edges become "Take the lift/stairs to floor n" and
//     bearings restart on the new floor.
//  5. Straight runs merge into one "Walk N m" line; turns name the landmark
//     where they happen.
#ifndef VR_NAVIGATOR_HPP
#define VR_NAVIGATOR_HPP

#include "HospitalMap.hpp"

#include <iosfwd>
#include <string>
#include <vector>

struct Route {
    std::string origin;
    std::string destination;
    std::vector<std::string> steps;
    std::vector<int> path;        // vertex ids, origin first
    double distance = 0.0;        // metres walked (lift rides excluded)
    int seconds = 0;              // at Navigator::kWalkingSpeed
};

std::ostream& operator<<(std::ostream& os, const Route& route);

class Navigator {
public:
    static constexpr double kWalkingSpeed = 1.2;   // metres per second

    explicit Navigator(const HospitalMap& map);

    // Shortest route from -> to. `destinationLabel` names the end point
    // ("ICU-2, Bed 7"; defaults to the node name) and `side` is 'L', 'R' or
    // ' ' for the final "on your left/right". Throws NoRouteFoundException.
    Route directions(int from, int to, const std::string& destinationLabel = "",
                     char side = ' ') const;
    // Same, for a path that is already known.
    Route describe(const std::vector<int>& path, const std::string& destinationLabel = "",
                   char side = ' ') const;

    static double bearing(const vr_node& a, const vr_node& b);   // degrees, -180..180
    static double normalise(double degrees);                      // into (-180, 180]
    static std::string classifyTurn(double delta);
    static std::string compass(double bearing);                   // "north-east"

private:
    const HospitalMap& map_;
};

#endif // VR_NAVIGATOR_HPP
