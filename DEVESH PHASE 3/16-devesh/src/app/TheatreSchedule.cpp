// TheatreSchedule.cpp
#include "TheatreSchedule.hpp"

#include <cctype>
#include <sstream>

namespace {

std::string trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos)
        return "";
    return s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
}

bool startsWith(const std::string& s, const std::string& prefix)
{
    return s.compare(0, prefix.size(), prefix) == 0;
}

} // namespace

std::vector<TheatreDayInfo> readTheatreSchedule(const OTScheduler& ot)
{
    std::vector<TheatreDayInfo> days;
    for (const std::string& name : ot.theatres()) {
        TheatreDayInfo day;
        day.name = name;
        day.freeCount = ot.freeSlots(name);
        for (int i = 0; i < ot.slotCount(name); ++i) {
            TheatreSlotInfo slot;
            slot.time = ot.slotTime(name, i);
            slot.patientId = ot.occupant(name, i);
            slot.booked = !slot.patientId.empty();
            day.slots.push_back(slot);
        }
        days.push_back(day);
    }

    // Procedures and ring order from the printed schedule.
    std::ostringstream os;
    ot.printSchedule(os);
    std::istringstream in(os.str());
    std::string raw;
    int current = -1;
    while (std::getline(in, raw)) {
        const std::string line = trim(raw);
        for (std::size_t t = 0; t < days.size(); ++t)
            if (startsWith(line, days[t].name + "  ("))
                current = static_cast<int>(t);
        if (current < 0)
            continue;
        TheatreDayInfo& day = days[static_cast<std::size_t>(current)];
        if (startsWith(line, "next free in cycle order:")) {
            std::istringstream nums(line.substr(line.find(':') + 1));
            std::string part;
            while (nums >> part) {
                if (!part.empty() && std::isdigit(static_cast<unsigned char>(part[0])))
                    day.ringOrder.push_back(std::stoi(part));
            }
            continue;
        }
        // "3. 10:00-11:00  VR-2026-000004  Angiography"
        const std::size_t dot = line.find(". ");
        if (dot == std::string::npos || dot == 0)
            continue;
        int number = 0;
        try {
            number = std::stoi(line.substr(0, dot));
        } catch (...) {
            continue;
        }
        if (number < 1 || number > static_cast<int>(day.slots.size()))
            continue;
        TheatreSlotInfo& slot = day.slots[static_cast<std::size_t>(number - 1)];
        if (!slot.booked)
            continue;
        const std::string prefix = slot.time + "  " + slot.patientId + "  ";
        const std::size_t at = line.find(prefix);
        if (at != std::string::npos)
            slot.procedure = line.substr(at + prefix.size());
    }
    return days;
}
