// ResourceRouter.cpp
#include "ResourceRouter.hpp"

#include "Exceptions.hpp"

#include <fstream>
#include <sstream>

ResourceRouter::ResourceRouter(const HospitalMap& map) : map_(map)
{
}

bool ResourceRouter::parseUnit(const std::string& text, CareUnit& out)
{
    if (text == "ICU") {
        out = CareUnit::ICU;
        return true;
    }
    if (text == "WARD") {
        out = CareUnit::Ward;
        return true;
    }
    return false;
}

void ResourceRouter::addBed(const std::string& code, int node, int number, CareUnit unit,
                            char side)
{
    if (!map_.hasNode(node))
        throw std::invalid_argument("bed " + code + " refers to unknown node " +
                                    std::to_string(node));
    if (unit == CareUnit::None || (side != 'L' && side != 'R'))
        throw std::invalid_argument("bed " + code + " needs a unit (ICU/WARD) and side (L/R)");
    if (findBed(code))
        throw std::invalid_argument("duplicate bed code " + code);
    beds_.push_back(Bed{code, node, number, unit, side, ""});
}

void ResourceRouter::loadBeds(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open " + path);
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream fields(line);
        std::string tag, code, unitName, side;
        int node = 0, number = 0;
        CareUnit unit = CareUnit::None;
        if (!(fields >> tag) || tag != "BED")
            continue;
        try {
            if (!(fields >> code >> node >> number >> unitName >> side) ||
                !parseUnit(unitName, unit) || side.size() != 1)
                throw std::invalid_argument("expected: BED code node number ICU|WARD L|R");
            addBed(code, node, number, unit, side[0]);
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
}

int ResourceRouter::origin() const
{
    return origin_ >= 0 ? origin_ : map_.firstOfType(NODE_ER);
}

void ResourceRouter::setOrigin(int node)
{
    if (!map_.hasNode(node))
        throw std::invalid_argument("unknown origin node " + std::to_string(node));
    origin_ = node;
}

BedAssignment ResourceRouter::allocate(const std::string& patientId, CareUnit unit)
{
    if (unit == CareUnit::None)
        throw std::invalid_argument("patient " + patientId + " does not need a bed");

    // Stage 1 - clinical type first. Only beds of the required unit are
    // ever candidates, so distance is never compared across types.
    std::vector<Bed*> candidates;
    for (Bed& b : beds_)
        if (b.unit == unit && b.occupant.empty())
            candidates.push_back(&b);
    if (candidates.empty())
        throw BedUnavailableException("no free " + Patient::unitName(unit) + " bed");

    // Stage 2 - shortest walking distance among the type-valid beds.
    const std::vector<double> dist = map_.distancesFrom(origin());
    Bed* best = nullptr;
    double bestDist = 0.0;
    for (Bed* b : candidates) {
        const double d = dist[static_cast<std::size_t>(b->node)];
        if (d == VR_UNREACHABLE)
            continue;
        if (!best || d < bestDist) {      // strict: ties keep file order
            best = b;
            bestDist = d;
        }
    }
    if (!best)
        throw NoRouteFoundException("no free " + Patient::unitName(unit) +
                                    " bed can be reached from " + map_.node(origin()).name);

    best->occupant = patientId;
    return BedAssignment{best->code, labelOf(*best), best->node, best->side, best->unit, bestDist};
}

Bed& ResourceRouter::bed(const std::string& code)
{
    for (Bed& b : beds_)
        if (b.code == code)
            return b;
    throw std::invalid_argument("no bed with code " + code);
}

void ResourceRouter::release(const std::string& code)
{
    bed(code).occupant.clear();
}

void ResourceRouter::occupy(const std::string& code, const std::string& patientId)
{
    Bed& b = bed(code);
    if (!b.occupant.empty() && b.occupant != patientId)
        throw BedUnavailableException("bed " + code + " is already taken by " + b.occupant);
    b.occupant = patientId;
}

void ResourceRouter::releaseAll()
{
    for (Bed& b : beds_)
        b.occupant.clear();
}

const Bed* ResourceRouter::findBed(const std::string& code) const
{
    for (const Bed& b : beds_)
        if (b.code == code)
            return &b;
    return nullptr;
}

std::string ResourceRouter::labelOf(const Bed& b) const
{
    return std::string(map_.node(b.node).name) + ", Bed " + std::to_string(b.number);
}

int ResourceRouter::freeBeds(CareUnit unit) const
{
    int n = 0;
    for (const Bed& b : beds_)
        if (b.unit == unit && b.occupant.empty())
            ++n;
    return n;
}
