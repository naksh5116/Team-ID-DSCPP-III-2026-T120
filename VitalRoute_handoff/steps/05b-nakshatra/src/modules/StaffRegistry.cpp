// StaffRegistry.cpp
#include "StaffRegistry.hpp"

#include "vr_sort.h"

#include <algorithm>
#include <climits>
#include <fstream>
#include <sstream>
#include <stdexcept>

int StaffRegistry::compareSlots(const void* a, const void* b)
{
    const Slot* x = static_cast<const Slot*>(a);
    const Slot* y = static_cast<const Slot*>(b);
    if (x->specialty != y->specialty)
        return x->specialty < y->specialty ? -1 : 1;
    if (x->nextFree != y->nextFree)
        return x->nextFree < y->nextFree ? -1 : 1;
    return (x->doctor > y->doctor) - (x->doctor < y->doctor);
}

namespace {

std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> out;
    std::string field;
    std::istringstream in(line);
    while (std::getline(in, field, '|'))
        out.push_back(field);
    return out;
}

std::string trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return "";
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

} // namespace

void StaffRegistry::checkUnique(const std::string& id) const
{
    if (find(id))
        throw std::invalid_argument("duplicate staff ID " + id);
}

Doctor& StaffRegistry::addDoctor(std::unique_ptr<Doctor> doctor)
{
    checkUnique(doctor->id());
    int code = specialtyCode(doctor->specialty());
    if (code < 0) {
        specialtyNames_.push_back(doctor->specialty());
        code = static_cast<int>(specialtyNames_.size()) - 1;
    }
    Doctor& ref = *doctor;
    doctors_.push_back(doctor.get());
    staff_.push_back(std::move(doctor));

    const Slot slot{code, ref.nextFree(), static_cast<int>(doctors_.size()) - 1};
    const std::size_t pos = vr_lower_bound(slots_.data(), slots_.size(), sizeof(Slot),
                                           &slot, compareSlots);
    slots_.insert(slots_.begin() + static_cast<std::ptrdiff_t>(pos), slot);
    return ref;
}

Nurse& StaffRegistry::addNurse(std::unique_ptr<Nurse> nurse)
{
    checkUnique(nurse->id());
    Nurse& ref = *nurse;
    nurses_.push_back(nurse.get());
    staff_.push_back(std::move(nurse));
    return ref;
}

Admin& StaffRegistry::addAdmin(std::unique_ptr<Admin> admin)
{
    checkUnique(admin->id());
    Admin& ref = *admin;
    admins_.push_back(admin.get());
    staff_.push_back(std::move(admin));
    return ref;
}

void StaffRegistry::loadRoster(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open staff roster " + path);

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        std::vector<std::string> f = splitFields(line);
        for (auto& s : f)
            s = trim(s);
        try {
            // role|id|name|age|phone|department|shift|...
            if (f.size() < 8)
                throw std::invalid_argument("too few fields");
            const int age = std::stoi(f[3]);
            if (f[0] == "D" && f.size() == 9) {
                addDoctor(std::unique_ptr<Doctor>(new Doctor(
                    f[1], f[2], age, f[4], f[5], f[6], f[7], std::stod(f[8]))));
            } else if (f[0] == "N" && f.size() == 8) {
                addNurse(std::unique_ptr<Nurse>(new Nurse(f[1], f[2], age, f[4], f[5], f[6], f[7])));
            } else if (f[0] == "A" && f.size() == 8) {
                addAdmin(std::unique_ptr<Admin>(new Admin(f[1], f[2], age, f[4], f[5], f[6], f[7])));
            } else {
                throw std::invalid_argument("unknown role or wrong field count");
            }
        } catch (const std::exception& e) {
            throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
}

void StaffRegistry::clear()
{
    slots_.clear();
    specialtyNames_.clear();
    doctors_.clear();
    nurses_.clear();
    admins_.clear();
    staff_.clear();
}

Staff* StaffRegistry::find(const std::string& id) const
{
    for (const auto& s : staff_)
        if (s->id() == id)
            return s.get();
    return nullptr;
}

Doctor* StaffRegistry::findDoctor(const std::string& id) const
{
    for (Doctor* d : doctors_)
        if (d->id() == id)
            return d;
    return nullptr;
}

int StaffRegistry::specialtyCode(const std::string& name) const
{
    for (std::size_t i = 0; i < specialtyNames_.size(); ++i)
        if (specialtyNames_[i] == name)
            return static_cast<int>(i);
    return -1;
}

std::size_t StaffRegistry::firstSlotOf(int code) const
{
    const Slot key{code, INT_MIN, INT_MIN};
    return vr_lower_bound(slots_.data(), slots_.size(), sizeof(Slot), &key, compareSlots);
}

Doctor* StaffRegistry::nextAvailable(const std::string& specialty) const
{
    const int code = specialtyCode(specialty);
    if (code < 0)
        return nullptr;
    const std::size_t pos = firstSlotOf(code);
    if (pos == slots_.size() || slots_[pos].specialty != code)
        return nullptr;
    return doctors_[static_cast<std::size_t>(slots_[pos].doctor)];
}

std::pair<Doctor*, int> StaffRegistry::assign(const std::string& specialty, int now, int minutes)
{
    const int code = specialtyCode(specialty);
    if (code < 0)
        return {nullptr, now};
    const std::size_t pos = firstSlotOf(code);
    if (pos == slots_.size() || slots_[pos].specialty != code)
        return {nullptr, now};

    Slot slot = slots_[pos];
    Doctor* doctor = doctors_[static_cast<std::size_t>(slot.doctor)];
    const int start = doctor->beginConsultation(now, minutes);

    // Move the entry to where its new next-free time belongs.
    slots_.erase(slots_.begin() + static_cast<std::ptrdiff_t>(pos));
    slot.nextFree = doctor->nextFree();
    const std::size_t to = vr_lower_bound(slots_.data(), slots_.size(), sizeof(Slot),
                                          &slot, compareSlots);
    slots_.insert(slots_.begin() + static_cast<std::ptrdiff_t>(to), slot);
    return {doctor, start};
}

void StaffRegistry::refreshAvailability()
{
    for (Slot& s : slots_)
        s.nextFree = doctors_[static_cast<std::size_t>(s.doctor)]->nextFree();
    vr_merge_sort(slots_.data(), slots_.size(), sizeof(Slot), compareSlots);
}

std::vector<const Doctor*> StaffRegistry::availability() const
{
    std::vector<const Doctor*> out;
    for (const Slot& s : slots_)
        out.push_back(doctors_[static_cast<std::size_t>(s.doctor)]);
    return out;
}

std::vector<std::string> StaffRegistry::specialties() const
{
    std::vector<std::string> out = specialtyNames_;
    std::sort(out.begin(), out.end());
    return out;
}
