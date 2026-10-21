// ResourceRouter.hpp - chooses the bed an emergency patient is sent to.
//
// Beds come from data/resources.txt; each sits at a ward or ICU node of the
// floor plan. Allocation uses Dijkstra distance from the Central Emergency
// Room to pick the nearest free bed.
#ifndef VR_RESOURCE_ROUTER_HPP
#define VR_RESOURCE_ROUTER_HPP

#include "HospitalMap.hpp"
#include "Patient.hpp"   // CareUnit

#include <string>
#include <vector>

struct Bed {
    std::string code;       // ICU2-07
    int node;               // ward / ICU node on the floor plan
    int number;             // bed number shown to people
    CareUnit unit;          // ICU or WARD
    char side;              // 'L' or 'R' on arrival
    std::string occupant;   // patient ID, empty when free
};

struct BedAssignment {
    std::string code;
    std::string label;      // "ICU-2, Bed 7"
    int node;
    char side;
    CareUnit unit;
    double distance;        // Dijkstra distance from the origin
};

class ResourceRouter {
public:
    explicit ResourceRouter(const HospitalMap& map);

    // Reads the BED lines of data/resources.txt (throws std::runtime_error).
    void loadBeds(const std::string& path);
    void addBed(const std::string& code, int node, int number, CareUnit unit, char side);

    // Where routes start: the first ER node unless set explicitly.
    int origin() const;
    void setOrigin(int node);

    // Reserves the nearest free bed of the required clinical unit for the
    // patient. Throws BedUnavailableException when none is free and
    // NoRouteFoundException when none of the free ones can be reached.
    BedAssignment allocate(const std::string& patientId, CareUnit unit);
    void release(const std::string& code);
    void occupy(const std::string& code, const std::string& patientId);
    void releaseAll();

    const Bed* findBed(const std::string& code) const;
    std::string labelOf(const Bed& bed) const;
    const std::vector<Bed>& beds() const { return beds_; }
    int freeBeds(CareUnit unit) const;

    static bool parseUnit(const std::string& text, CareUnit& out);

private:
    Bed& bed(const std::string& code);

    const HospitalMap& map_;
    std::vector<Bed> beds_;
    int origin_ = -1;
};

#endif // VR_RESOURCE_ROUTER_HPP
