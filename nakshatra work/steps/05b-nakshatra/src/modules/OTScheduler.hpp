// OTScheduler.hpp - daily operation theatre slots.
//
// Each theatre has a fixed table of slots for the day and a circular queue
// (vr_cqueue) holding the numbers of the slots that are still free.
// Booking dequeues the next free slot in cycle order; releasing enqueues it
// again, so it is reusable at once and no other slot moves.
#ifndef VR_OT_SCHEDULER_HPP
#define VR_OT_SCHEDULER_HPP

#include "vr_cqueue.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class OTScheduler {
public:
    struct Booking {
        std::string theatre;
        int slot;           // 0-based
        int startMinute;    // minutes since midnight
    };

    OTScheduler() = default;
    OTScheduler(const OTScheduler&) = delete;
    OTScheduler& operator=(const OTScheduler&) = delete;

    void addTheatre(const std::string& name, int nodeId, int slots, int firstHour,
                    int slotMinutes = 60);
    // Reads the "OT" lines of data/resources.txt.
    void loadTheatres(const std::string& path);

    // Throws SlotUnavailableException when the theatre is fully booked or
    // unknown.
    Booking book(const std::string& theatre, const std::string& patientId,
                 const std::string& procedure);
    // Throws SlotUnavailableException when the slot is not currently booked.
    void release(const std::string& theatre, int slot);

    std::vector<std::string> theatres() const;
    int freeSlots(const std::string& theatre) const;
    int slotCount(const std::string& theatre) const;
    std::string slotTime(const std::string& theatre, int slot) const;
    // Patient booked into a slot ("" when free).
    std::string occupant(const std::string& theatre, int slot) const;
    void printSchedule(std::ostream& os) const;

private:
    struct Cell {
        bool booked = false;
        std::string patientId;
        std::string procedure;
    };
    struct Theatre {
        Theatre() = default;
        ~Theatre() { vr_cq_free(&freeRing); }
        Theatre(const Theatre&) = delete;
        Theatre& operator=(const Theatre&) = delete;

        std::string name;
        int node = 0;
        int firstHour = 8;
        int slotMinutes = 60;
        std::vector<Cell> cells;
        vr_cqueue freeRing{};
    };

    Theatre& theatre(const std::string& name);
    const Theatre& theatre(const std::string& name) const;

    std::vector<std::unique_ptr<Theatre>> theatres_;
};

#endif // VR_OT_SCHEDULER_HPP
