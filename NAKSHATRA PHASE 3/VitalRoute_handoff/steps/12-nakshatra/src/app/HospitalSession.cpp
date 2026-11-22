// HospitalSession.cpp - application logic shared by the console and the GUI.
#include "HospitalSession.hpp"

#include "Exceptions.hpp"

#include <algorithm>
#include <cstdio>
#include <new>
#include <stdexcept>
#include <utility>

const char* const HospitalSession::kEmergencySpecialty = "Emergency Medicine";

HospitalSession::HospitalSession(std::string dataDir)
    : dataDir_(std::move(dataDir)), router_(map_), navigator_(map_),
      store_(dataDir_ + "/records")
{
    if (vr_stack_init(&undo_, sizeof(UndoEntry), 8) != 0)
        throw std::bad_alloc();
}

HospitalSession::~HospitalSession()
{
    vr_stack_free(&undo_);
}

// ------------------------------------------------------------ start-up --

HospitalSession::StartupReport HospitalSession::startUp()
{
    map_.load(dataDir_ + "/hospital_graph.txt");
    router_.loadBeds(dataDir_ + "/resources.txt");
    theatres_.loadTheatres(dataDir_ + "/resources.txt");

    StartupReport report;
    if (store_.hasSavedRecords()) {
        report.loadedFromRecords = true;
        report.loaded = store_.load(patients_, staff_, billing_);
        report.warnings = restoreRuntimeState();
    } else {
        staff_.loadRoster(dataDir_ + "/doctors.txt");
        seed();
    }
    return report;
}

Patient& HospitalSession::admit(const std::string& name, int age, const std::string& phone,
                                PatientKind kind, int severity, const std::string& complaint,
                                const std::string& department)
{
    const int today = vrclock::today();
    Patient& p = patients_.registerPatient(name, age, phone, kind, severity, today, complaint,
                                           department);
    billing_.open(p.id(), today);
    if (kind == PatientKind::Emergency)
        triage_.admitEmergency(p);
    else
        triage_.joinOpd(p);
    return p;
}

void HospitalSession::seed()
{
    const int today = vrclock::today();

    // Three earlier admissions, already discharged, so the date index and
    // the bill listings have some history.
    auto history = [&](const std::string& name, int age, int severity, int daysAgo, int stay,
                       const std::string& complaint, const std::string& bedLabel,
                       CareUnit unit, const std::string& doctor) -> Patient& {
        const int in = vrclock::addDays(today, -daysAgo);
        const int out = vrclock::addDays(in, stay);
        Patient& p = patients_.registerPatient(name, age, "", PatientKind::Emergency, severity,
                                               in, complaint, kEmergencySpecialty);
        p.setStatus(PatientStatus::Discharged);
        p.setDischargeDate(out);
        p.addVisit(VisitRecord::make(in, doctor, "Emergency (ESI " + std::to_string(severity) +
                                                     "): " + complaint,
                                     "Admitted to " + bedLabel));
        p.addVisit(VisitRecord::make(out, "Discharge desk", "Discharged", "Bill settled"));
        billing_.open(p.id(), in);
        billing_.charge(p.id(), ProcedureCharge("Emergency consultation - " + doctor, in, 1500.0,
                                                500.0, 250.0));
        billing_.charge(p.id(), BedCharge(Patient::unitName(unit) + " bed - " + bedLabel, in,
                                          unit == CareUnit::ICU ? kIcuBedPerDay : kWardBedPerDay,
                                          stay));
        return p;
    };
    const Patient& h1 = history("Gurpreet Kaur", 55, 2, 41, 4, "Diabetic ketoacidosis",
                                "ICU-2, Bed 6", CareUnit::ICU, "Dr. Arjun Khanna");
    history("Arnav Bose", 19, 3, 17, 2, "Dehydration after gastroenteritis",
            "General Ward A, Bed 2", CareUnit::Ward, "Dr. Meera Iyer");
    history("Sunita Yadav", 63, 3, 6, 3, "Pneumonia", "General Ward C, Bed 1", CareUnit::Ward,
            "Dr. Meera Iyer");
    billing_.attachInsurance(h1.id(), Insurance("New India Assurance", "NIA-20931",
                                                Insurance::Kind::Flat, 15000.0));
    for (const Patient* p : patients_.all())
        billing_.settle(p->id(), p->dischargeDate());

    // Waiting now: six emergencies arriving in this order, and two outpatients.
    Patient& e1 = admit("Suresh Kumar", 67, "98110-20001", PatientKind::Emergency, 1,
                        "Cardiac arrest, pulse restored in ambulance", kEmergencySpecialty);
    admit("Ananya Sharma", 24, "98110-20002", PatientKind::Emergency, 4,
          "Suspected fractured forearm", kEmergencySpecialty);
    admit("Mohammed Imran", 45, "98110-20003", PatientKind::Emergency, 2,
          "Severe asthma attack, poor response to inhaler", kEmergencySpecialty);
    admit("Lakshmi Pillai", 72, "98110-20004", PatientKind::Emergency, 1,
          "Stroke symptoms, onset 40 minutes ago", kEmergencySpecialty);
    admit("Vikram Singh", 33, "98110-20005", PatientKind::Emergency, 3,
          "Abdominal pain with vomiting", kEmergencySpecialty);
    admit("Neha Joshi", 29, "98110-20006", PatientKind::Emergency, 5,
          "Migraine", kEmergencySpecialty);
    Patient& o1 = admit("Ramesh Gupta", 61, "98110-20007", PatientKind::Outpatient, 0,
                        "Blood pressure follow-up", "General Medicine");
    admit("Pooja Menon", 36, "98110-20008", PatientKind::Outpatient, 0,
          "Knee pain after running", "Orthopaedics");

    billing_.attachInsurance(e1.id(), Insurance("Star Health", "SH-448120",
                                                Insurance::Kind::Percentage, 20.0, 50000.0));
    billing_.attachInsurance(o1.id(), Insurance("CGHS", "CG-77031", Insurance::Kind::Flat, 500.0));

    // A returning outpatient: the older record is added second and lands at
    // the head of the visit list.
    o1.addVisit(VisitRecord::make(vrclock::addDays(today, -30), "Dr. Farah Siddiqui",
                                  "Hypertension, stage 1", "Amlodipine 5 mg once daily"));
    o1.addVisit(VisitRecord::make(vrclock::addDays(today, -90), "Dr. Farah Siddiqui",
                                  "Raised blood pressure on screening",
                                  "Lifestyle advice; recheck in 8 weeks"));
}

std::vector<std::string> HospitalSession::restoreRuntimeState()
{
    std::vector<std::string> warnings;
    triage_.rebuild(patients_.all());
    for (Patient* p : patients_.all()) {
        if (p->status() != PatientStatus::Admitted || p->bedCode().empty())
            continue;
        try {
            router_.occupy(p->bedCode(), p->id());
        } catch (const std::exception& e) {
            warnings.push_back(e.what());
        }
    }
    return warnings;
}

PersistenceManager::Counts HospitalSession::save()
{
    return store_.save(patients_, staff_, billing_);
}

// ----------------------------------------------------------- reception --

void HospitalSession::pushUndo(UndoKind kind, const std::string& patientId,
                               const std::string& note)
{
    UndoEntry e{};
    e.kind = kind;
    std::snprintf(e.patientId, sizeof e.patientId, "%s", patientId.c_str());
    const Bill* b = billing_.find(patientId);
    e.chargeCount = b ? static_cast<unsigned long>(b->chargeCount()) : 0UL;
    std::snprintf(e.note, sizeof e.note, "%s", note.c_str());
    if (vr_stack_push(&undo_, &e) != 0)
        throw std::bad_alloc();
}

Patient& HospitalSession::registerEmergency(const std::string& name, int age,
                                            const std::string& phone,
                                            const std::string& complaint, int severity)
{
    Patient::checkSeverity(severity);   // throws InvalidSeverityException
    Patient& p = admit(name, age, phone, PatientKind::Emergency, severity, complaint,
                       kEmergencySpecialty);
    pushUndo(UNDO_REGISTRATION, p.id(), "registration of " + p.name());
    return p;
}

Patient& HospitalSession::registerOpd(const std::string& name, int age, const std::string& phone,
                                      const std::string& complaint,
                                      const std::string& department)
{
    Patient& p = admit(name, age, phone, PatientKind::Outpatient, 0, complaint, department);
    pushUndo(UNDO_REGISTRATION, p.id(), "registration of " + p.name());
    return p;
}

std::vector<std::string> HospitalSession::opdDepartments() const
{
    std::vector<std::string> depts;
    for (const std::string& s : staff_.specialties())
        if (s != kEmergencySpecialty)
            depts.push_back(s);
    return depts;
}

std::vector<const Patient*> HospitalSession::emergencyOrder() const
{
    return triage_.emergencyOrder(patients_);
}

std::size_t HospitalSession::emergencyPosition(const std::string& patientId) const
{
    const std::vector<const Patient*> order = emergencyOrder();
    for (std::size_t i = 0; i < order.size(); ++i)
        if (order[i]->id() == patientId)
            return i + 1;
    return 0;
}

void HospitalSession::attachInsurance(const std::string& patientId, const Insurance& cover)
{
    billing_.attachInsurance(patientId, cover);
}

// ----------------------------------------------------------- treatment --

HospitalSession::TreatResult HospitalSession::treatNext(
    const std::string& diagnosis, const std::string& prescriptions,
    const std::optional<MedicineLine>& medicine)
{
    if (triage_.hasEmergency())
        return treatNextEmergency();

    TreatResult r;
    if (!triage_.hasOpd())
        return r;   // NobodyWaiting
    const OpdVisit visit = beginOpd();
    r.outcome = TreatResult::Outcome::Outpatient;
    r.patient = visit.patient;
    r.doctor = visit.doctor;
    r.doctorStart = visit.doctorStart;
    r.doctorName = visit.doctorName;
    r.billSoFar = completeOpd(visit, diagnosis, prescriptions, medicine);
    return r;
}

HospitalSession::TreatResult HospitalSession::treatNextEmergency()
{
    TreatResult r;
    const std::string id = triage_.nextEmergency();
    Patient& p = patients_.find(id);
    r.patient = &p;
    const int today = vrclock::today();

    const CareUnit unit = p.requiredUnit();
    if (unit != CareUnit::None) {
        try {
            r.bed = router_.allocate(p.id(), unit);
        } catch (const std::exception& e) {
            triage_.requeueEmergency(p);     // keeps their place at the front
            r.outcome = TreatResult::Outcome::NoBed;
            r.error = e.what();
            return r;
        }
    }

    const auto assigned = staff_.assign(kEmergencySpecialty, vrclock::minutesNow(), 30);
    r.doctor = assigned.first;
    r.doctorStart = assigned.second;
    r.doctorName = r.doctor ? r.doctor->name() : std::string("duty doctor");
    if (r.doctor)
        p.setDoctorId(r.doctor->id());

    // Charges accrue as resources are used.
    billing_.charge(p.id(), TestCharge("Emergency panel (ECG, bloods)", today, 1800.0,
                                       p.severity() <= 2));
    billing_.charge(p.id(), ProcedureCharge("Emergency consultation - " + r.doctorName, today,
                                            r.doctor ? r.doctor->consultationFee() : 1500.0,
                                            500.0, 250.0));

    if (unit == CareUnit::None) {
        p.setStatus(PatientStatus::Treated);
        p.addVisit(VisitRecord::make(today, r.doctorName,
                                     "ER (ESI " + std::to_string(p.severity()) +
                                         "): " + p.complaint(),
                                     "Treated in the ER and released"));
        r.outcome = TreatResult::Outcome::ReleasedFromEr;
        return r;
    }

    p.assignBed(r.bed.code, r.bed.label);
    p.setStatus(PatientStatus::Admitted);
    const double rate = unit == CareUnit::ICU ? kIcuBedPerDay : kWardBedPerDay;
    billing_.charge(p.id(), BedCharge(Patient::unitName(unit) + " bed - " + r.bed.label, today,
                                      rate, 1));
    p.addVisit(VisitRecord::make(today, r.doctorName,
                                 "Emergency (ESI " + std::to_string(p.severity()) + "): " +
                                     p.complaint(),
                                 "Admitted to " + r.bed.label));
    r.outcome = TreatResult::Outcome::Admitted;
    try {
        r.route = navigator_.directions(router_.origin(), r.bed.node, r.bed.label, r.bed.side);
    } catch (const NoRouteFoundException& e) {
        r.routeError = e.what();
    }
    return r;
}

HospitalSession::OpdVisit HospitalSession::beginOpd()
{
    OpdVisit v;
    const std::string id = triage_.nextOpd();
    Patient& p = patients_.find(id);
    v.patient = &p;

    auto assigned = staff_.assign(p.department(), vrclock::minutesNow(), 15);
    if (!assigned.first)
        assigned = staff_.assign("General Medicine", vrclock::minutesNow(), 15);
    v.doctor = assigned.first;
    v.doctorStart = assigned.second;
    v.doctorName = v.doctor ? v.doctor->name() : std::string("duty doctor");
    if (v.doctor)
        p.setDoctorId(v.doctor->id());
    return v;
}

double HospitalSession::completeOpd(const OpdVisit& visit, const std::string& diagnosis,
                                    const std::string& prescriptions,
                                    const std::optional<MedicineLine>& medicine)
{
    Patient& p = *visit.patient;
    const int today = vrclock::today();
    billing_.charge(p.id(), ProcedureCharge("OPD consultation - " + visit.doctorName, today,
                                            visit.doctor ? visit.doctor->consultationFee() : 600.0,
                                            100.0, 0.0));
    if (medicine)
        billing_.charge(p.id(), MedicineCharge(medicine->name, today, medicine->unitPrice,
                                               medicine->quantity));
    p.addVisit(VisitRecord::make(today, visit.doctorName,
                                 diagnosis.empty() ? "Routine review" : diagnosis,
                                 prescriptions.empty() ? "None" : prescriptions));
    p.setStatus(PatientStatus::Treated);
    return billing_.billFor(p.id()).projectedTotal();
}

// ------------------------------------------------------ patient record --

Patient& HospitalSession::findPatient(const std::string& id)
{
    return patients_.find(id);
}

const Patient& HospitalSession::findPatient(const std::string& id) const
{
    return patients_.find(id);
}

const Doctor* HospitalSession::findDoctor(const std::string& id) const
{
    return staff_.findDoctor(id);
}

const Staff* HospitalSession::findStaff(const std::string& id) const
{
    return staff_.find(id);
}

void HospitalSession::addVisitNote(const std::string& patientId, int date,
                                   const std::string& doctor, const std::string& diagnosis,
                                   const std::string& prescriptions)
{
    Patient& p = patients_.find(patientId);
    p.addVisit(VisitRecord::make(date, doctor, diagnosis,
                                 prescriptions.empty() ? "None" : prescriptions));
}

Route HospitalSession::routeForPatient(const std::string& patientId) const
{
    const Patient& p = patients_.find(patientId);
    if (p.bedCode().empty())
        throw std::runtime_error(p.id() + " has no bed assigned");
    const Bed* bed = router_.findBed(p.bedCode());
    if (!bed)
        throw std::runtime_error("unknown bed " + p.bedCode());
    return navigator_.directions(router_.origin(), bed->node, p.bedLabel(), bed->side);
}

std::vector<Patient*> HospitalSession::admittedBetween(int fromYmd, int toYmd) const
{
    if (fromYmd > toYmd)
        std::swap(fromYmd, toYmd);
    return patients_.admittedBetween(fromYmd, toYmd);
}

// --------------------------------------------------------------- staff --

std::vector<std::string> HospitalSession::specialties() const
{
    return staff_.specialties();
}

const Doctor* HospitalSession::nextAvailableDoctor(const std::string& specialty) const
{
    return staff_.nextAvailable(specialty);
}

// ---------------------------------------------- theatres and transfers --

OTScheduler::Booking HospitalSession::bookTheatre(const std::string& theatre,
                                                  const std::string& patientId,
                                                  const std::string& procedure,
                                                  double surgeonFee)
{
    Patient& p = patients_.find(patientId);
    const OTScheduler::Booking b = theatres_.book(theatre, p.id(), procedure);
    billing_.charge(p.id(), ProcedureCharge("Surgery: " + procedure + " (" + theatre + ")",
                                            vrclock::today(), surgeonFee, 8000.0, 3500.0));
    return b;
}

void HospitalSession::releaseTheatreSlot(const std::string& theatre, int slot)
{
    theatres_.release(theatre, slot);
}

TransferRequest HospitalSession::requestTransfer(bool urgent, bool ambulance,
                                                 const std::string& patientId,
                                                 const std::string& from, const std::string& to)
{
    if (!patientId.empty())
        patients_.find(patientId);   // throws PatientNotFoundException
    return transfers_.request(urgent, ambulance, patientId, from, to);
}

HospitalSession::DispatchResult HospitalSession::dispatchNextTransfer()
{
    DispatchResult d;
    if (!transfers_.dispatchNext(d.request))
        return d;
    d.dispatched = true;
    const TransferRequest& r = d.request;
    if (r.ambulance && r.patientId[0]) {
        const Bill* b = billing_.find(r.patientId);
        if (b && !b->settled()) {
            billing_.charge(r.patientId, ProcedureCharge("Ambulance transfer " +
                                                             std::string(r.code),
                                                         vrclock::today(), 0.0, 2500.0, 300.0));
            d.chargedBillId = b->billId();
        }
    }
    return d;
}

bool HospitalSession::cancelLastRoutineTransfer(TransferRequest& out)
{
    return transfers_.cancelLastRoutine(out);
}

// ------------------------------------------------------------- billing --

Bill& HospitalSession::billFor(const std::string& patientId)
{
    return billing_.billFor(patientId);
}

const Bill* HospitalSession::findBill(const std::string& patientId) const
{
    return billing_.find(patientId);
}

std::vector<const Bill*> HospitalSession::listBills(bool byAmount) const
{
    return billing_.listBills(byAmount ? BillingEngine::SortKey::Amount
                                       : BillingEngine::SortKey::Date);
}

void HospitalSession::addManualCharge(const std::string& patientId, const Charge& charge,
                                      const std::string& authoriserId)
{
    const Staff* admin = staff_.find(authoriserId);
    if (!admin)
        throw std::runtime_error("no staff member with ID " + authoriserId);
    Bill& bill = billing_.billFor(patientId);
    billing_.manualCharge(patientId, charge, *admin);   // throws unless an admin
    pushUndo(UNDO_CHARGE, patientId,
             "charge '" + charge.description() + "' on " + bill.billId());
}

// ----------------------------------------------------------- discharge --

HospitalSession::DischargeBlock HospitalSession::dischargeBlocker(
    const std::string& patientId) const
{
    const Patient& p = patients_.find(patientId);
    if (p.status() == PatientStatus::Waiting)
        return DischargeBlock::StillWaiting;
    if (p.status() == PatientStatus::Discharged)
        return DischargeBlock::AlreadyDischarged;
    return DischargeBlock::None;
}

HospitalSession::BedRelease HospitalSession::prepareDischarge(const std::string& patientId,
                                                              int date)
{
    if (dischargeBlocker(patientId) != DischargeBlock::None)
        throw std::logic_error(patientId + " cannot be discharged now");
    Patient& p = patients_.find(patientId);
    BedRelease r;
    if (date < p.admissionDate()) {
        r.dateClamped = true;
        date = p.admissionDate();
    }
    r.date = date;

    if (!p.bedCode().empty()) {
        const Bed* bed = router_.findBed(p.bedCode());
        const int nights = std::max(1, vrclock::daysBetween(p.admissionDate(), date));
        if (bed && nights > 1) {
            const double rate = bed->unit == CareUnit::ICU ? kIcuBedPerDay : kWardBedPerDay;
            billing_.charge(p.id(), BedCharge(Patient::unitName(bed->unit) + " bed - " +
                                                  p.bedLabel() + " (further days)",
                                              date, rate, nights - 1));
        }
        router_.release(p.bedCode());
        r.releasedBedLabel = p.bedLabel();
        p.clearBed();
    }
    return r;
}

const Bill& HospitalSession::settleDischarge(const std::string& patientId, int date)
{
    Patient& p = patients_.find(patientId);
    Bill& bill = billing_.billFor(p.id());
    billing_.settle(p.id(), date);           // insurance applied here, once
    p.setStatus(PatientStatus::Discharged);
    p.setDischargeDate(date);
    p.addVisit(VisitRecord::make(date, "Discharge desk", "Discharged",
                                 "Bill " + bill.billId() + " settled"));
    return bill;
}

const Bill& HospitalSession::discharge(const std::string& patientId, int date,
                                       const std::optional<Insurance>& cover)
{
    const BedRelease r = prepareDischarge(patientId, date);
    if (cover)
        attachInsurance(patientId, *cover);
    return settleDischarge(patientId, r.date);
}

// ---------------------------------------------------------------- undo --

HospitalSession::UndoResult HospitalSession::undoLast()
{
    UndoResult r;
    UndoEntry e;
    if (vr_stack_pop(&undo_, &e) != 0)
        return r;   // Nothing
    r.patientId = e.patientId;
    r.note = e.note;
    if (e.kind == UNDO_REGISTRATION) {
        if (!triage_.withdraw(e.patientId)) {
            r.kind = UndoResult::Kind::RegistrationDropped;
            return r;
        }
        billing_.remove(e.patientId);
        patients_.remove(e.patientId);
        r.kind = UndoResult::Kind::RegistrationUndone;
    } else {
        Bill* b = billing_.find(e.patientId);
        if (!b || b->settled() || b->chargeCount() != e.chargeCount) {
            r.kind = UndoResult::Kind::ChargeDropped;
            return r;
        }
        std::unique_ptr<Charge> removed = billing_.undoLastCharge(e.patientId);
        r.amount = removed ? removed->calculateCost() : 0.0;
        r.kind = UndoResult::Kind::ChargeUndone;
    }
    return r;
}

bool HospitalSession::canUndo() const
{
    return !vr_stack_is_empty(&undo_);
}

std::string HospitalSession::nextUndoNote() const
{
    UndoEntry e;
    if (vr_stack_peek(&undo_, &e) != 0)
        return "";
    return e.note;
}
