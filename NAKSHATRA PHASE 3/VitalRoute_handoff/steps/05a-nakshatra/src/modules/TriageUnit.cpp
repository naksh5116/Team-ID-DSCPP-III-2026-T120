// TriageUnit.cpp
#include "TriageUnit.hpp"

#include "PatientRegistry.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

TriageUnit::TriageUnit()
{
    if (vr_pq_init(&heap_, 16) != 0)
        throw std::bad_alloc();
    vr_queue_init(&opd_, sizeof(OpdTicket));
}

TriageUnit::~TriageUnit()
{
    vr_pq_free(&heap_);
    vr_queue_free(&opd_);
}

vr_pq_item TriageUnit::keyFor(const Patient& patient)
{
    vr_pq_item item;
    item.key = 6.0 - patient.severity();              // ESI 1 -> 5 (highest)
    item.tiebreak = -static_cast<double>(patient.arrivalSeq());
    item.value = patient.arrivalSeq();
    return item;
}

void TriageUnit::admitEmergency(Patient& patient)
{
    Patient::checkSeverity(patient.severity());
    patient.setArrivalSeq(nextSeq_++);
    requeueEmergency(patient);
}

void TriageUnit::requeueEmergency(const Patient& patient)
{
    if (vr_pq_insert(&heap_, keyFor(patient)) != 0)
        throw std::bad_alloc();
    emergencyIds_[patient.arrivalSeq()] = patient.id();
}

void TriageUnit::joinOpd(Patient& patient)
{
    OpdTicket t{};
    patient.setArrivalSeq(nextSeq_++);
    std::snprintf(t.patientId, sizeof t.patientId, "%s", patient.id().c_str());
    t.seq = patient.arrivalSeq();
    if (vr_queue_enqueue(&opd_, &t) != 0)
        throw std::bad_alloc();
}

bool TriageUnit::hasEmergency() const { return !vr_pq_is_empty(&heap_); }
bool TriageUnit::hasOpd() const { return !vr_queue_is_empty(&opd_); }
std::size_t TriageUnit::emergencyCount() const { return vr_pq_size(&heap_); }
std::size_t TriageUnit::opdCount() const { return vr_queue_size(&opd_); }

std::string TriageUnit::peekEmergency() const
{
    vr_pq_item top;
    if (vr_pq_peek(&heap_, &top) != 0)
        return "";
    return emergencyIds_.at(top.value);
}

std::string TriageUnit::nextEmergency()
{
    vr_pq_item top;
    if (vr_pq_extract_max(&heap_, &top) != 0)
        return "";
    const auto it = emergencyIds_.find(top.value);
    std::string id = it->second;
    emergencyIds_.erase(it);
    return id;
}

std::string TriageUnit::nextOpd()
{
    OpdTicket t;
    if (vr_queue_dequeue(&opd_, &t) != 0)
        return "";
    return t.patientId;
}

bool TriageUnit::withdraw(const std::string& patientId)
{
    for (auto it = emergencyIds_.begin(); it != emergencyIds_.end(); ++it) {
        if (it->second == patientId) {
            vr_pq_remove_value(&heap_, it->first);
            emergencyIds_.erase(it);
            return true;
        }
    }
    return vr_queue_remove_first(&opd_, [](const void* elem, void* ctx) {
        return std::strcmp(static_cast<const OpdTicket*>(elem)->patientId,
                           static_cast<const std::string*>(ctx)->c_str()) == 0 ? 1 : 0;
    }, const_cast<std::string*>(&patientId)) == 0;
}

std::vector<std::string> TriageUnit::opdLine() const
{
    std::vector<std::string> out;
    for (std::size_t i = 0; i < vr_queue_size(&opd_); ++i)
        out.push_back(static_cast<const OpdTicket*>(vr_queue_at(&opd_, i))->patientId);
    return out;
}

std::vector<const Patient*> TriageUnit::emergencyOrder(const PatientRegistry& registry) const
{
    std::vector<const Patient*> out;
    for (const auto& entry : emergencyIds_)
        if (const Patient* p = registry.tryFind(entry.second))
            out.push_back(p);
    std::sort(out.begin(), out.end(),
              [](const Patient* a, const Patient* b) { return *b < *a; });
    return out;
}

void TriageUnit::rebuild(const std::vector<Patient*>& patients)
{
    clear();
    std::vector<vr_pq_item> items;
    std::vector<Patient*> opd;
    for (Patient* p : patients) {
        nextSeq_ = std::max(nextSeq_, p->arrivalSeq() + 1);
        if (p->status() != PatientStatus::Waiting)
            continue;
        if (p->isEmergency()) {
            items.push_back(keyFor(*p));
            emergencyIds_[p->arrivalSeq()] = p->id();
        } else {
            opd.push_back(p);
        }
    }
    if (vr_pq_heapify(&heap_, items.data(), items.size()) != 0)
        throw std::bad_alloc();

    std::sort(opd.begin(), opd.end(), [](const Patient* a, const Patient* b) {
        return a->arrivalSeq() < b->arrivalSeq();
    });
    for (Patient* p : opd) {
        OpdTicket t{};
        std::snprintf(t.patientId, sizeof t.patientId, "%s", p->id().c_str());
        t.seq = p->arrivalSeq();
        if (vr_queue_enqueue(&opd_, &t) != 0)
            throw std::bad_alloc();
    }
}

void TriageUnit::clear()
{
    while (vr_pq_extract_max(&heap_, nullptr) == 0) {
    }
    vr_queue_free(&opd_);
    emergencyIds_.clear();
}
