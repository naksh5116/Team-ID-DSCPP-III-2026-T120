// TriageUnit.hpp - who is seen next.
//
//   Emergency line: the C max-heap (vr_pqueue) keyed on
//                   (6 - severity, -arrival_seq), so ESI 1 leaves first and
//                   equal severities leave in arrival order.
//   OPD line:       the C FIFO queue (vr_queue).
#ifndef VR_TRIAGE_UNIT_HPP
#define VR_TRIAGE_UNIT_HPP

#include "Patient.hpp"
#include "vr_pqueue.h"
#include "vr_queue.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

class PatientRegistry;

class TriageUnit {
public:
    TriageUnit();
    ~TriageUnit();
    TriageUnit(const TriageUnit&) = delete;
    TriageUnit& operator=(const TriageUnit&) = delete;

    // Validates the severity (InvalidSeverityException), stamps the arrival
    // sequence and places the patient in the emergency heap.
    void admitEmergency(Patient& patient);
    // Stamps the arrival sequence and joins the OPD line.
    void joinOpd(Patient& patient);
    // Puts a patient back with their original arrival sequence (used when
    // no suitable bed was free).
    void requeueEmergency(const Patient& patient);

    bool hasEmergency() const;
    bool hasOpd() const;
    std::size_t emergencyCount() const;
    std::size_t opdCount() const;

    std::string peekEmergency() const;     // "" when empty
    std::string nextEmergency();           // extract-max; "" when empty
    std::string nextOpd();                 // dequeue; "" when empty

    // Takes a waiting patient out of whichever line they are in (undo).
    bool withdraw(const std::string& patientId);

    std::vector<std::string> opdLine() const;
    // Waiting emergencies, most urgent first, ordered with Patient::operator<.
    std::vector<const Patient*> emergencyOrder(const PatientRegistry& registry) const;

    // Rebuilds both lines from saved records: every WAITING patient goes back
    // in (the heap in O(n) with vr_pq_heapify, the OPD line in arrival order)
    // and new arrivals are numbered after the highest saved sequence.
    void rebuild(const std::vector<Patient*>& patients);
    void clear();

private:
    struct OpdTicket {
        char patientId[24];
        long seq;
    };

    static vr_pq_item keyFor(const Patient& patient);

    vr_pqueue heap_;
    vr_queue opd_;
    long nextSeq_ = 1;
    std::map<long, std::string> emergencyIds_;   // arrival seq -> patient ID
};

#endif // VR_TRIAGE_UNIT_HPP
