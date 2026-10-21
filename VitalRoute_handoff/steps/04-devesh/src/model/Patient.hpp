// Patient.hpp - a registered patient and their visit history.
//
// The visit history is the C engine's doubly linked list (vr_dlist) holding
// VisitRecord values; Patient owns it and follows the rule of five.
#ifndef VR_PATIENT_HPP
#define VR_PATIENT_HPP

#include "Person.hpp"
#include "vr_dlist.h"

#include <cstddef>
#include <string>
#include <vector>

enum class PatientKind { Emergency, Outpatient };
enum class PatientStatus { Waiting, Admitted, Treated, Discharged };
// Where an emergency patient must be admitted, decided by severity.
enum class CareUnit { None, Ward, ICU };

// Plain, trivially copyable record so the C list can store it by value.
struct VisitRecord {
    int  date;                 // YYYYMMDD
    char doctor[48];
    char diagnosis[96];
    char prescriptions[160];

    static VisitRecord make(int date, const std::string& doctor,
                            const std::string& diagnosis, const std::string& prescriptions);
};

class Patient : public Person {
public:
    Patient(std::string id, std::string name, int age, std::string phone,
            PatientKind kind, int severity, int admissionDate,
            std::string complaint, std::string department);
    ~Patient() override;
    Patient(const Patient& other);
    Patient& operator=(const Patient& other);
    Patient(Patient&& other) noexcept;
    Patient& operator=(Patient&& other) noexcept;

    std::string role() const override;
    void describe(std::ostream& os) const override;

    // Throws InvalidSeverityException unless 1 <= severity <= 5.
    static void checkSeverity(int severity);

    PatientKind kind() const { return kind_; }
    bool isEmergency() const { return kind_ == PatientKind::Emergency; }
    int severity() const { return severity_; }        // 0 for outpatients
    // ESI 1-2 need intensive care, 3 a ward bed, 4-5 are treated in the ER.
    CareUnit requiredUnit() const;

    long arrivalSeq() const { return arrivalSeq_; }
    void setArrivalSeq(long seq) { arrivalSeq_ = seq; }
    int admissionDate() const { return admissionDate_; }
    PatientStatus status() const { return status_; }
    void setStatus(PatientStatus s) { status_ = s; }
    const std::string& complaint() const { return complaint_; }
    const std::string& department() const { return department_; }

    const std::string& bedCode() const { return bedCode_; }
    const std::string& bedLabel() const { return bedLabel_; }
    void assignBed(const std::string& code, const std::string& label);
    void clearBed();
    const std::string& doctorId() const { return doctorId_; }
    void setDoctorId(const std::string& id) { doctorId_ = id; }
    int dischargeDate() const { return dischargeDate_; }
    void setDischargeDate(int ymd) { dischargeDate_ = ymd; }

    // Visit history, kept in date order. Records dated on/before the first
    // visit go in at the head and on/after the last at the tail (both O(1));
    // anything in between is walked into place.
    void addVisit(const VisitRecord& visit);
    std::size_t visitCount() const;
    std::vector<VisitRecord> visitsOldestFirst() const;   // forward walk
    std::vector<VisitRecord> visitsNewestFirst() const;   // backward walk

    // Triage ordering: a < b when a is LESS urgent than b (lower ESI
    // priority, or the same ESI level but a later arrival). Outpatients rank
    // below every emergency.
    bool operator<(const Patient& other) const;

    static std::string kindName(PatientKind k);
    static std::string statusName(PatientStatus s);
    static std::string unitName(CareUnit u);

private:
    int urgencyRank() const;   // 1 (most urgent) .. 6 (outpatient)
    void copyVisitsFrom(const Patient& other);

    PatientKind kind_;
    int severity_;
    long arrivalSeq_ = 0;
    int admissionDate_;
    PatientStatus status_ = PatientStatus::Waiting;
    std::string complaint_;
    std::string department_;
    std::string bedCode_;
    std::string bedLabel_;
    std::string doctorId_;
    int dischargeDate_ = 0;
    vr_dlist visits_;
};

#endif // VR_PATIENT_HPP
