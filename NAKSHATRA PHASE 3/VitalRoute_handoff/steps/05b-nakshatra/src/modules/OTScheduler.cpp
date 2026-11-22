// OTScheduler.cpp
#include "OTScheduler.hpp"

#include "Exceptions.hpp"
#include "Person.hpp"   // vrclock::formatTime

#include <fstream>
#include <iomanip>
#include <new>
#include <ostream>
#include <sstream>

void OTScheduler::addTheatre(const std::string& name, int nodeId, int slots, int firstHour,
                             int slotMinutes)
{
    if (slots < 1 || slotMinutes < 1 || firstHour < 0 || firstHour > 23)
        throw std::invalid_argument("bad theatre definition for " + name);
    for (const auto& t : theatres_)
        if (t->name == name)
            throw std::invalid_argument("duplicate theatre " + name);

    std::unique_ptr<Theatre> t(new Theatre);
    t->name = name;
    t->node = nodeId;
    t->firstHour = firstHour;
    t->slotMinutes = slotMinutes;
    t->cells.resize(static_cast<std::size_t>(slots));
    if (vr_cq_init(&t->freeRing, static_cast<std::size_t>(slots), sizeof(int)) != 0)
        throw std::bad_alloc();
    for (int i = 0; i < slots; ++i)
        vr_cq_enqueue(&t->freeRing, &i);          // the whole day starts free
    theatres_.push_back(std::move(t));
}

void OTScheduler::loadTheatres(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open " + path);
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream fields(line);
        std::string tag, name;
        int node = 0, slots = 0, firstHour = 0;
        if (!(fields >> tag) || tag != "OT")
            continue;
        if (!(fields >> name >> node >> slots >> firstHour))
            throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": bad OT line");
        addTheatre(name, node, slots, firstHour);
    }
}

OTScheduler::Theatre& OTScheduler::theatre(const std::string& name)
{
    for (auto& t : theatres_)
        if (t->name == name)
            return *t;
    throw SlotUnavailableException("no operation theatre called " + name);
}

const OTScheduler::Theatre& OTScheduler::theatre(const std::string& name) const
{
    for (const auto& t : theatres_)
        if (t->name == name)
            return *t;
    throw SlotUnavailableException("no operation theatre called " + name);
}

OTScheduler::Booking OTScheduler::book(const std::string& theatreName,
                                       const std::string& patientId,
                                       const std::string& procedure)
{
    Theatre& t = theatre(theatreName);
    int slot = -1;
    if (vr_cq_dequeue(&t.freeRing, &slot) != 0)
        throw SlotUnavailableException(theatreName + " has no free slot left today");
    Cell& c = t.cells[static_cast<std::size_t>(slot)];
    c.booked = true;
    c.patientId = patientId;
    c.procedure = procedure;
    return {t.name, slot, t.firstHour * 60 + slot * t.slotMinutes};
}

void OTScheduler::release(const std::string& theatreName, int slot)
{
    Theatre& t = theatre(theatreName);
    if (slot < 0 || slot >= static_cast<int>(t.cells.size()))
        throw SlotUnavailableException("slot " + std::to_string(slot + 1) + " does not exist in " +
                                       theatreName);
    Cell& c = t.cells[static_cast<std::size_t>(slot)];
    if (!c.booked)
        throw SlotUnavailableException("slot " + std::to_string(slot + 1) + " in " + theatreName +
                                       " is not booked");
    c = Cell{};
    vr_cq_enqueue(&t.freeRing, &slot);            // cannot overflow: slot was taken out
}

std::vector<std::string> OTScheduler::theatres() const
{
    std::vector<std::string> out;
    for (const auto& t : theatres_)
        out.push_back(t->name);
    return out;
}

int OTScheduler::freeSlots(const std::string& name) const
{
    return static_cast<int>(vr_cq_size(&theatre(name).freeRing));
}

int OTScheduler::slotCount(const std::string& name) const
{
    return static_cast<int>(theatre(name).cells.size());
}

std::string OTScheduler::slotTime(const std::string& name, int slot) const
{
    const Theatre& t = theatre(name);
    const int start = t.firstHour * 60 + slot * t.slotMinutes;
    return vrclock::formatTime(start) + "-" + vrclock::formatTime(start + t.slotMinutes);
}

std::string OTScheduler::occupant(const std::string& name, int slot) const
{
    const Theatre& t = theatre(name);
    if (slot < 0 || slot >= static_cast<int>(t.cells.size()))
        return "";
    return t.cells[static_cast<std::size_t>(slot)].patientId;
}

void OTScheduler::printSchedule(std::ostream& os) const
{
    for (const auto& t : theatres_) {
        os << "\n " << t->name << "  (" << vr_cq_size(&t->freeRing) << " of " << t->cells.size()
           << " slots free; ring head=" << t->freeRing.head << " tail=" << t->freeRing.tail
           << " count=" << t->freeRing.count << ")\n";
        for (std::size_t i = 0; i < t->cells.size(); ++i) {
            const Cell& c = t->cells[i];
            os << "   " << std::setw(2) << i + 1 << ". "
               << slotTime(t->name, static_cast<int>(i)) << "  ";
            if (c.booked)
                os << c.patientId << "  " << c.procedure << '\n';
            else
                os << "free\n";
        }
        os << "   next free in cycle order:";
        if (vr_cq_is_empty(&t->freeRing))
            os << " none (fully booked)";
        for (std::size_t i = 0; i < vr_cq_size(&t->freeRing); ++i)
            os << ' ' << *static_cast<const int*>(vr_cq_at(&t->freeRing, i)) + 1;
        os << '\n';
    }
}
