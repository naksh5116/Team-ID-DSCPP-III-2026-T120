// Patient.cpp
#include "Patient.hpp"

#include "Exceptions.hpp"

#include <cstdio>
#include <new>
#include <ostream>
#include <type_traits>
#include <utility>

static_assert(std::is_trivially_copyable<VisitRecord>::value,
              "VisitRecord is copied byte-for-byte by the C list");

namespace {

template <std::size_t N>
void copyField(char (&dst)[N], const std::string& src)
{
    std::snprintf(dst, N, "%s", src.c_str());
}

} // namespace

VisitRecord VisitRecord::make(int date, const std::string& doctor,
                              const std::string& diagnosis, const std::string& prescriptions)
{
    VisitRecord v{};
    v.date = date;
    copyField(v.doctor, doctor);
    copyField(v.diagnosis, diagnosis);
    copyField(v.prescriptions, prescriptions);
    return v;
}

Patient::Patient(std::string id, std::string name, int age, std::string phone,
                 PatientKind kind, int severity, int admissionDate,
                 std::string complaint, std::string department)
    : Person(std::move(id), std::move(name), age, std::move(phone)),
      kind_(kind),
      severity_(kind == PatientKind::Emergency ? severity : 0),
      admissionDate_(admissionDate),
      complaint_(std::move(complaint)),
      department_(std::move(department))
{
    if (kind_ == PatientKind::Emergency)
        checkSeverity(severity_);
    if (!vrclock::validDate(admissionDate_))
        throw std::invalid_argument("invalid admission date " + std::to_string(admissionDate_));
    vr_dlist_init(&visits_, sizeof(VisitRecord));
}

Patient::~Patient()
{
    vr_dlist_free(&visits_);
}

Patient::Patient(const Patient& other)
    : Person(other),
      kind_(other.kind_), severity_(other.severity_), arrivalSeq_(other.arrivalSeq_),
      admissionDate_(other.admissionDate_), status_(other.status_),
      complaint_(other.complaint_), department_(other.department_),
      bedCode_(other.bedCode_), bedLabel_(other.bedLabel_), doctorId_(other.doctorId_),
      dischargeDate_(other.dischargeDate_)
{
    vr_dlist_init(&visits_, sizeof(VisitRecord));
    copyVisitsFrom(other);
}

Patient& Patient::operator=(const Patient& other)
{
    if (this != &other) {
        Patient copy(other);          // copy-and-swap keeps us safe if copying throws
        *this = std::move(copy);
    }
    return *this;
}

Patient::Patient(Patient&& other) noexcept
    : Person(std::move(other)),
      kind_(other.kind_), severity_(other.severity_), arrivalSeq_(other.arrivalSeq_),
      admissionDate_(other.admissionDate_), status_(other.status_),
      complaint_(std::move(other.complaint_)), department_(std::move(other.department_)),
      bedCode_(std::move(other.bedCode_)), bedLabel_(std::move(other.bedLabel_)),
      doctorId_(std::move(other.doctorId_)), dischargeDate_(other.dischargeDate_),
      visits_(other.visits_)
{
    vr_dlist_init(&other.visits_, sizeof(VisitRecord));   // other no longer owns the nodes
}

Patient& Patient::operator=(Patient&& other) noexcept
{
    if (this != &other) {
        Person::operator=(std::move(other));
        kind_ = other.kind_;
        severity_ = other.severity_;
        arrivalSeq_ = other.arrivalSeq_;
        admissionDate_ = other.admissionDate_;
        status_ = other.status_;
        complaint_ = std::move(other.complaint_);
        department_ = std::move(other.department_);
        bedCode_ = std::move(other.bedCode_);
        bedLabel_ = std::move(other.bedLabel_);
        doctorId_ = std::move(other.doctorId_);
        dischargeDate_ = other.dischargeDate_;
        vr_dlist_free(&visits_);
        visits_ = other.visits_;
        vr_dlist_init(&other.visits_, sizeof(VisitRecord));
    }
    return *this;
}

void Patient::copyVisitsFrom(const Patient& other)
{
    for (vr_dnode* n = vr_dlist_head(&other.visits_); n; n = vr_dnode_next(n)) {
        if (!vr_dlist_insert_tail(&visits_, vr_dnode_cdata(n))) {
            vr_dlist_free(&visits_);
            throw std::bad_alloc();
        }
    }
}

std::string Patient::role() const
{
    return isEmergency() ? "Emergency patient" : "Outpatient";
}

void Patient::describe(std::ostream& os) const
{
    Person::describe(os);
    if (isEmergency())
        os << ", ESI " << severity_;
    else
        os << ", " << department_;
    os << ", " << statusName(status_);
    if (!bedLabel_.empty())
        os << " (" << bedLabel_ << ")";
}

void Patient::checkSeverity(int severity)
{
    if (severity < 1 || severity > 5)
        throw InvalidSeverityException(severity);
}

CareUnit Patient::requiredUnit() const
{
    if (!isEmergency())
        return CareUnit::None;
    if (severity_ <= 2)
        return CareUnit::ICU;
    if (severity_ == 3)
        return CareUnit::Ward;
    return CareUnit::None;
}

void Patient::assignBed(const std::string& code, const std::string& label)
{
    bedCode_ = code;
    bedLabel_ = label;
}

void Patient::clearBed()
{
    bedCode_.clear();
    bedLabel_.clear();
}

void Patient::addVisit(const VisitRecord& visit)
{
    vr_dnode* head = vr_dlist_head(&visits_);
    vr_dnode* tail = vr_dlist_tail(&visits_);
    vr_dnode* placed = nullptr;

    if (!tail || static_cast<const VisitRecord*>(vr_dnode_cdata(tail))->date <= visit.date) {
        placed = vr_dlist_insert_tail(&visits_, &visit);          // the usual case
    } else if (static_cast<const VisitRecord*>(vr_dnode_cdata(head))->date > visit.date) {
        placed = vr_dlist_insert_head(&visits_, &visit);          // back-dated record
    } else {
        // Walk back from the tail to the last visit not later than this one.
        vr_dnode* n = tail;
        while (static_cast<const VisitRecord*>(vr_dnode_cdata(n))->date > visit.date)
            n = vr_dnode_prev(n);
        placed = vr_dlist_insert_after(&visits_, n, &visit);
    }
    if (!placed)
        throw std::bad_alloc();
}

std::size_t Patient::visitCount() const
{
    return vr_dlist_size(&visits_);
}

std::vector<VisitRecord> Patient::visitsOldestFirst() const
{
    std::vector<VisitRecord> out;
    out.reserve(visitCount());
    for (vr_dnode* n = vr_dlist_head(&visits_); n; n = vr_dnode_next(n))
        out.push_back(*static_cast<const VisitRecord*>(vr_dnode_cdata(n)));
    return out;
}

std::vector<VisitRecord> Patient::visitsNewestFirst() const
{
    std::vector<VisitRecord> out;
    out.reserve(visitCount());
    for (vr_dnode* n = vr_dlist_tail(&visits_); n; n = vr_dnode_prev(n))
        out.push_back(*static_cast<const VisitRecord*>(vr_dnode_cdata(n)));
    return out;
}

int Patient::urgencyRank() const
{
    return isEmergency() ? severity_ : 6;
}

bool Patient::operator<(const Patient& other) const
{
    if (urgencyRank() != other.urgencyRank())
        return urgencyRank() > other.urgencyRank();   // higher ESI number = less urgent
    return arrivalSeq_ > other.arrivalSeq_;           // later arrival = less urgent
}

std::string Patient::kindName(PatientKind k)
{
    return k == PatientKind::Emergency ? "EMERGENCY" : "OPD";
}

std::string Patient::statusName(PatientStatus s)
{
    switch (s) {
    case PatientStatus::Waiting:    return "WAITING";
    case PatientStatus::Admitted:   return "ADMITTED";
    case PatientStatus::Treated:    return "TREATED";
    case PatientStatus::Discharged: return "DISCHARGED";
    }
    return "?";
}

std::string Patient::unitName(CareUnit u)
{
    switch (u) {
    case CareUnit::ICU:  return "ICU";
    case CareUnit::Ward: return "WARD";
    case CareUnit::None: return "NONE";
    }
    return "?";
}
