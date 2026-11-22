// Navigator.cpp
#include "Navigator.hpp"

#include <cmath>
#include <ostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

std::string metres(double m)
{
    return std::to_string(std::lround(m)) + " m";
}

bool isVertical(EdgeType t)
{
    return t == EDGE_LIFT || t == EDGE_STAIRS;
}

// How a turn names the place it happens.
std::string landmark(const vr_node& n)
{
    return n.type == NODE_CORRIDOR ? std::string("the ") + n.name : std::string(n.name);
}

std::string floorName(int floor)
{
    return floor == 0 ? "the ground floor" : "floor " + std::to_string(floor);
}

} // namespace

Navigator::Navigator(const HospitalMap& map) : map_(map)
{
}

double Navigator::bearing(const vr_node& a, const vr_node& b)
{
    return std::atan2(b.y - a.y, b.x - a.x) * 180.0 / M_PI;
}

double Navigator::normalise(double degrees)
{
    double d = std::fmod(degrees, 360.0);
    if (d <= -180.0)
        d += 360.0;
    else if (d > 180.0)
        d -= 360.0;
    return d;
}

std::string Navigator::classifyTurn(double delta)
{
    const double a = std::fabs(delta);
    if (a < 20.0)
        return "Continue straight";
    const std::string side = delta > 0.0 ? "left" : "right";
    if (a < 60.0)
        return "Bear slightly " + side;
    if (a <= 135.0)
        return "Turn " + side;
    return "Turn sharply " + side;
}

std::string Navigator::compass(double bearing)
{
    static const char* const names[] = {"east", "north-east", "north", "north-west",
                                        "west", "south-west", "south", "south-east"};
    double b = std::fmod(bearing + 360.0 + 22.5, 360.0);
    return names[static_cast<int>(b / 45.0) % 8];
}

Route Navigator::directions(int from, int to, const std::string& destinationLabel,
                            char side) const
{
    return describe(map_.shortestPath(from, to), destinationLabel, side);
}

Route Navigator::describe(const std::vector<int>& path, const std::string& destinationLabel,
                          char side) const
{
    Route r;
    r.path = path;
    if (path.empty())
        return r;
    r.origin = map_.node(path.front()).name;
    r.destination = destinationLabel.empty() ? std::string(map_.node(path.back()).name)
                                             : destinationLabel;

    double leg = 0.0;              // metres in the current straight run
    std::string legCorridor;       // first corridor the run passes along
    bool haveBearing = false;      // false at the origin and after a floor change
    double prevBearing = 0.0;
    EdgeType lastVertical = EDGE_LIFT;

    auto flushLeg = [&]() {
        if (leg > 0.0) {
            std::string s = "Walk " + metres(leg);
            if (!legCorridor.empty())
                s += " along the " + legCorridor;
            r.steps.push_back(s);
        }
        leg = 0.0;
        legCorridor.clear();
    };

    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        const vr_node& a = map_.node(path[i]);
        const vr_node& b = map_.node(path[i + 1]);
        const vr_edge* e = map_.edge(a.id, b.id);

        if (isVertical(e->type)) {
            // Ride/climb as many floors as the path does in one instruction.
            flushLeg();
            std::size_t j = i + 1;
            double climbed = e->weight;
            while (j + 1 < path.size()) {
                const vr_edge* next = map_.edge(path[j], path[j + 1]);
                if (next->type != e->type)
                    break;
                climbed += next->weight;
                ++j;
            }
            const int floor = map_.node(path[j]).floor;
            r.steps.push_back(std::string(e->type == EDGE_LIFT ? "Take the lift to "
                                                               : "Take the stairs to ") +
                              floorName(floor));
            if (e->type == EDGE_STAIRS)
                r.distance += climbed;         // stairs are walked, lifts are ridden
            lastVertical = e->type;
            haveBearing = false;               // bearings restart on the new floor
            i = j - 1;
            continue;
        }

        const double brg = bearing(a, b);
        if (i == 0) {
            r.steps.push_back("From the " + std::string(a.name) + ", head towards " + b.name);
        } else if (!haveBearing) {
            r.steps.push_back(std::string(lastVertical == EDGE_LIFT ? "Exit the lift"
                                                                    : "Leave the stairwell") +
                              " heading " + compass(brg) + " towards " + b.name);
        } else {
            const std::string turn = classifyTurn(normalise(brg - prevBearing));
            if (turn != "Continue straight") {
                flushLeg();
                r.steps.push_back(turn + " at " + landmark(a));
            }
        }
        leg += e->weight;
        r.distance += e->weight;
        if (legCorridor.empty() && b.type == NODE_CORRIDOR)
            legCorridor = b.name;
        prevBearing = brg;
        haveBearing = true;
    }
    flushLeg();

    std::string arrive = "Arrive at " + r.destination;
    if (side == 'L' || side == 'R')
        arrive += std::string(" - on your ") + (side == 'L' ? "left" : "right");
    r.steps.push_back(arrive);
    r.seconds = static_cast<int>(std::lround(r.distance / kWalkingSpeed));
    return r;
}

std::ostream& operator<<(std::ostream& os, const Route& route)
{
    const std::string rule(60, '-');
    os << "ROUTE: " << route.origin << "  ->  " << route.destination << '\n' << rule << '\n';
    for (std::size_t i = 0; i < route.steps.size(); ++i)
        os << (i + 1 < 10 ? " " : "") << i + 1 << ". " << route.steps[i] << '\n';
    os << rule << '\n'
       << "Total distance: " << metres(route.distance)
       << "      Estimated walking time: " << route.seconds << " s";
    if (route.seconds >= 60)
        os << " (" << route.seconds / 60 << " min " << route.seconds % 60 << " s)";
    os << '\n';
    return os;
}
