// StaffRegistry.hpp - the staff roster and "next available doctor".
//
// Doctors sit in an availability array kept sorted by
// (specialty, next-free time). A lower-bound binary search (vr_lower_bound)
// for (specialty, earliest possible time) lands on the soonest-free doctor
// of that specialty in O(log n); after a booking the doctor's entry is
// moved to its new sorted position, again found by lower bound.
#ifndef VR_STAFF_REGISTRY_HPP
#define VR_STAFF_REGISTRY_HPP

#include "Staff.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class StaffRegistry {
public:
    StaffRegistry() = default;
    StaffRegistry(const StaffRegistry&) = delete;
    StaffRegistry& operator=(const StaffRegistry&) = delete;

    Doctor& addDoctor(std::unique_ptr<Doctor> doctor);
    Nurse& addNurse(std::unique_ptr<Nurse> nurse);
    Admin& addAdmin(std::unique_ptr<Admin> admin);

    // Reads a roster file (data/doctors.txt). Throws std::runtime_error on
    // a malformed line.
    void loadRoster(const std::string& path);
    void clear();

    Staff* find(const std::string& id) const;
    Doctor* findDoctor(const std::string& id) const;

    // Soonest-free doctor of a specialty, or nullptr if there is none.
    Doctor* nextAvailable(const std::string& specialty) const;
    // Books that doctor for `minutes` starting no earlier than `now` and
    // re-sorts the array. Returns the doctor and the start time; the doctor
    // is nullptr when nobody has that specialty.
    std::pair<Doctor*, int> assign(const std::string& specialty, int now, int minutes);
    // Call after changing a doctor's next-free time directly (loading).
    void refreshAvailability();

    std::vector<const Doctor*> availability() const;   // array order
    std::vector<std::string> specialties() const;       // sorted, unique

    const std::vector<Doctor*>& doctors() const { return doctors_; }
    const std::vector<Nurse*>& nurses() const { return nurses_; }
    const std::vector<Admin*>& admins() const { return admins_; }
    std::size_t size() const { return staff_.size(); }

private:
    struct Slot {
        int specialty;    // index into specialtyNames_
        int nextFree;     // minutes since midnight
        int doctor;       // index into doctors_
    };

    // Orders slots by (specialty, next-free time, doctor index).
    static int compareSlots(const void* a, const void* b);
    int specialtyCode(const std::string& name) const;   // -1 if unknown
    std::size_t firstSlotOf(int code) const;             // lower bound
    void checkUnique(const std::string& id) const;

    std::vector<std::unique_ptr<Staff>> staff_;          // owns everyone
    std::vector<Doctor*> doctors_;
    std::vector<Nurse*> nurses_;
    std::vector<Admin*> admins_;
    std::vector<std::string> specialtyNames_;
    std::vector<Slot> slots_;                            // kept sorted
};

#endif // VR_STAFF_REGISTRY_HPP
