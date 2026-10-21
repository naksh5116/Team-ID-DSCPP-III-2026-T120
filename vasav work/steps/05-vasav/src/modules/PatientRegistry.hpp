// PatientRegistry.hpp - owns every Patient and indexes them twice:
//   - vr_hash  : patient ID -> Patient*      (O(1) lookup)
//   - vr_avl   : admission date -> Patient*  (date-range queries)
#ifndef VR_PATIENT_REGISTRY_HPP
#define VR_PATIENT_REGISTRY_HPP

#include "Patient.hpp"
#include "vr_avl.h"
#include "vr_hash.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class PatientRegistry {
public:
    PatientRegistry();
    ~PatientRegistry();
    PatientRegistry(const PatientRegistry&) = delete;
    PatientRegistry& operator=(const PatientRegistry&) = delete;

    // Creates a patient with the next "VR-<year>-<6 digits>" identifier.
    Patient& registerPatient(const std::string& name, int age, const std::string& phone,
                             PatientKind kind, int severity, int admissionDate,
                             const std::string& complaint, const std::string& department);
    // Takes ownership of an already-identified patient (used when loading
    // saved records). Throws std::invalid_argument on a duplicate/bad ID.
    Patient& adopt(std::unique_ptr<Patient> patient);

    Patient& find(const std::string& id);               // throws PatientNotFoundException
    const Patient& find(const std::string& id) const;
    Patient* tryFind(const std::string& id) const;      // nullptr when absent
    bool contains(const std::string& id) const;
    void remove(const std::string& id);                 // throws PatientNotFoundException
    void clear();

    // Everyone admitted on dates lo..hi inclusive, in date order.
    std::vector<Patient*> admittedBetween(int fromYmd, int toYmd) const;
    // Every patient, merge-sorted by admission date then ID.
    std::vector<Patient*> all() const;

    std::size_t size() const;
    vr_hash_stats hashStats() const;
    const vr_avl& dateIndex() const { return byDate_; }

    static std::string makeId(int year, long serial);
    static bool parseId(const std::string& id, int& year, long& serial);

private:
    void index(Patient* p);
    void deleteAll();

    vr_hash byId_;
    vr_avl byDate_;
    long nextSerial_ = 1;
};

#endif // VR_PATIENT_REGISTRY_HPP
