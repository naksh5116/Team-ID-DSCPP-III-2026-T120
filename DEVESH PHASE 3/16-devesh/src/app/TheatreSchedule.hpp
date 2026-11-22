// TheatreSchedule.hpp - one day of every operation theatre, as plain data.
//
// OTScheduler exposes the booked procedure and the free-slot ring order only
// through printSchedule(), and src/modules may not change. This reads that
// printout (with occupant(), slotTime() and freeSlots() for the rest) -
// read-only use of the public interface. Used by both front ends.
#ifndef VR_THEATRE_SCHEDULE_HPP
#define VR_THEATRE_SCHEDULE_HPP

#include "OTScheduler.hpp"

#include <string>
#include <vector>

struct TheatreSlotInfo {
    std::string time;        // 08:00-09:00
    bool booked = false;
    std::string patientId;
    std::string procedure;
};

struct TheatreDayInfo {
    std::string name;
    std::vector<TheatreSlotInfo> slots;
    int freeCount = 0;
    std::vector<int> ringOrder;   // free slot numbers (1-based) in circular-queue order
};

std::vector<TheatreDayInfo> readTheatreSchedule(const OTScheduler& ot);

#endif // VR_THEATRE_SCHEDULE_HPP
