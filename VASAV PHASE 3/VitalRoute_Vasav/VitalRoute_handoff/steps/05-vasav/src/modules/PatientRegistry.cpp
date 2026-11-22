// PatientRegistry.cpp
#include "PatientRegistry.hpp"

#include "Exceptions.hpp"
#include "vr_sort.h"

#include <cstdio>
#include <new>
#include <stdexcept>

PatientRegistry::PatientRegistry()
{
    if (vr_hash_init(&byId_, 16) != 0)
        throw std::bad_alloc();
    vr_avl_init(&byDate_);
}

PatientRegistry::~PatientRegistry()
{
    deleteAll();
    vr_hash_free(&byId_);
    vr_avl_free(&byDate_);
}

void PatientRegistry::deleteAll()
{
    // The registry owns the patients; the indexes only borrow them.
    vr_hash_foreach(&byId_, [](const char*, void* value, void*) {
        delete static_cast<Patient*>(value);
    }, nullptr);
}

void PatientRegistry::clear()
{
    deleteAll();
    vr_hash_free(&byId_);
    if (vr_hash_init(&byId_, 16) != 0)
        throw std::bad_alloc();
    vr_avl_free(&byDate_);
    nextSerial_ = 1;
}

std::string PatientRegistry::makeId(int year, long serial)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "VR-%04d-%06ld", year, serial);
    return buf;
}

bool PatientRegistry::parseId(const std::string& id, int& year, long& serial)
{
    int consumed = 0;
    if (id.size() != 14 ||
        std::sscanf(id.c_str(), "VR-%4d-%6ld%n", &year, &serial, &consumed) != 2 ||
        consumed != 14)
        return false;
    return year >= 1900 && serial > 0;
}

void PatientRegistry::index(Patient* p)
{
    if (vr_hash_put(&byId_, p->id().c_str(), p) < 0)
        throw std::bad_alloc();
    if (vr_avl_insert(&byDate_, p->admissionDate(), p) != 0) {
        vr_hash_remove(&byId_, p->id().c_str());
        throw std::bad_alloc();
    }
}

Patient& PatientRegistry::registerPatient(const std::string& name, int age,
                                          const std::string& phone, PatientKind kind,
                                          int severity, int admissionDate,
                                          const std::string& complaint,
                                          const std::string& department)
{
    std::unique_ptr<Patient> p(new Patient(makeId(admissionDate / 10000, nextSerial_), name,
                                           age, phone, kind, severity, admissionDate,
                                           complaint, department));
    return adopt(std::move(p));
}

Patient& PatientRegistry::adopt(std::unique_ptr<Patient> patient)
{
    int year = 0;
    long serial = 0;
    if (!parseId(patient->id(), year, serial))
        throw std::invalid_argument("malformed patient ID '" + patient->id() + "'");
    if (contains(patient->id()))
        throw std::invalid_argument("duplicate patient ID " + patient->id());

    Patient* raw = patient.get();
    index(raw);
    patient.release();            // now owned through the hash index
    if (serial >= nextSerial_)
        nextSerial_ = serial + 1;
    return *raw;
}

Patient* PatientRegistry::tryFind(const std::string& id) const
{
    return static_cast<Patient*>(vr_hash_get(&byId_, id.c_str()));
}

Patient& PatientRegistry::find(const std::string& id)
{
    Patient* p = tryFind(id);
    if (!p)
        throw PatientNotFoundException(id);
    return *p;
}

const Patient& PatientRegistry::find(const std::string& id) const
{
    const Patient* p = tryFind(id);
    if (!p)
        throw PatientNotFoundException(id);
    return *p;
}

bool PatientRegistry::contains(const std::string& id) const
{
    return vr_hash_contains(&byId_, id.c_str()) != 0;
}

void PatientRegistry::remove(const std::string& id)
{
    Patient* p = static_cast<Patient*>(vr_hash_remove(&byId_, id.c_str()));
    if (!p)
        throw PatientNotFoundException(id);
    vr_avl_remove(&byDate_, p->admissionDate(), p);
    delete p;
}

std::vector<Patient*> PatientRegistry::admittedBetween(int fromYmd, int toYmd) const
{
    std::vector<Patient*> out;
    vr_avl_range(&byDate_, fromYmd, toYmd, [](int, void* value, void* ctx) {
        static_cast<std::vector<Patient*>*>(ctx)->push_back(static_cast<Patient*>(value));
    }, &out);
    return out;
}

std::vector<Patient*> PatientRegistry::all() const
{
    std::vector<Patient*> out;
    out.reserve(size());
    vr_hash_foreach(&byId_, [](const char*, void* value, void* ctx) {
        static_cast<std::vector<Patient*>*>(ctx)->push_back(static_cast<Patient*>(value));
    }, &out);
    // Hash order is arbitrary; merge sort by admission date, then ID.
    vr_merge_sort(out.data(), out.size(), sizeof(Patient*), [](const void* a, const void* b) {
        const Patient* x = *static_cast<Patient* const*>(a);
        const Patient* y = *static_cast<Patient* const*>(b);
        if (x->admissionDate() != y->admissionDate())
            return x->admissionDate() < y->admissionDate() ? -1 : 1;
        return x->id().compare(y->id());
    });
    return out;
}

std::size_t PatientRegistry::size() const
{
    return vr_hash_size(&byId_);
}

vr_hash_stats PatientRegistry::hashStats() const
{
    vr_hash_stats st;
    vr_hash_get_stats(&byId_, &st);
    return st;
}
