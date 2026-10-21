// main.cpp - VitalRoute console.
//
// Wires the modules together behind the twelve-option menu. The undo
// journal (last reception or billing entry) is a vr_stack from the C engine.
#include "BillingEngine.hpp"
#include "Exceptions.hpp"
#include "HospitalMap.hpp"
#include "Navigator.hpp"
#include "OTScheduler.hpp"
#include "PatientRegistry.hpp"
#include "PersistenceManager.hpp"
#include "ResourceRouter.hpp"
#include "StaffRegistry.hpp"
#include "TransferDesk.hpp"
#include "TriageUnit.hpp"
#include "vr_stack.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

const double kIcuBedPerDay = 6000.0;
const double kWardBedPerDay = 2500.0;
const char* const kEmergencySpecialty = "Emergency Medicine";

// ------------------------------------------------------------ input --

struct InputClosed {};   // stdin reached end-of-file

std::string trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return "";
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::string ask(const std::string& prompt)
{
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line))
        throw InputClosed{};
    return trim(line);
}

std::string askRequired(const std::string& prompt)
{
    for (;;) {
        std::string s = ask(prompt);
        if (!s.empty())
            return s;
        std::cout << "  This field is required.\n";
    }
}

bool parseInt(const std::string& s, long& out)
{
    if (s.empty())
        return false;
    char* end = nullptr;
    out = std::strtol(s.c_str(), &end, 10);
    return *end == '\0';
}

int askInt(const std::string& prompt, int lo, int hi)
{
    for (;;) {
        long v = 0;
        if (parseInt(ask(prompt), v) && v >= lo && v <= hi)
            return static_cast<int>(v);
        std::cout << "  Please enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

double askAmount(const std::string& prompt)
{
    for (;;) {
        const std::string s = ask(prompt);
        char* end = nullptr;
        const double v = std::strtod(s.c_str(), &end);
        if (!s.empty() && *end == '\0' && v >= 0.0)
            return v;
        std::cout << "  Please enter an amount of zero or more.\n";
    }
}

bool askYes(const std::string& prompt)
{
    const std::string s = ask(prompt + " (y/n): ");
    return !s.empty() && (s[0] == 'y' || s[0] == 'Y');
}

int askDate(const std::string& prompt, int fallback)
{
    for (;;) {
        const std::string s = ask(prompt + " [" + vrclock::formatDate(fallback) + "]: ");
        if (s.empty())
            return fallback;
        if (const int d = vrclock::parseDate(s))
            return d;
        std::cout << "  Dates look like 2026-09-30 or 20260930.\n";
    }
}

// Numbered pick list; returns the chosen index.
std::size_t choose(const std::string& title, const std::vector<std::string>& options)
{
    std::cout << "  " << title << '\n';
    for (std::size_t i = 0; i < options.size(); ++i)
        std::cout << "    " << i + 1 << ". " << options[i] << '\n';
    return static_cast<std::size_t>(askInt("  Choice: ", 1, static_cast<int>(options.size())) - 1);
}

std::string money(double v)
{
    std::ostringstream os;
    os << "Rs " << std::fixed << std::setprecision(2) << v;
    return os.str();
}

// ------------------------------------------------------------- undo --

enum UndoKind { UNDO_REGISTRATION = 1, UNDO_CHARGE = 2 };

struct UndoEntry {            // plain struct: stored by value in vr_stack
    int kind;
    char patientId[24];
    unsigned long chargeCount;  // bill size right after a charge entry
    char note[96];
};

// ------------------------------------------------------------- app --

class VitalRouteApp {
public:
    explicit VitalRouteApp(std::string dataDir);
    ~VitalRouteApp();
    VitalRouteApp(const VitalRouteApp&) = delete;
    VitalRouteApp& operator=(const VitalRouteApp&) = delete;

    void run();

private:
    void startUp();
    void seed();
    void restoreRuntimeState();
    Patient& admit(const std::string& name, int age, const std::string& phone, PatientKind kind,
                   int severity, const std::string& complaint, const std::string& department);

    void registerEmergency();
    void registerOpd();
    void treatNext();
    void treatEmergency();
    void treatOpd();
    void lookUp();
    void admittedBetween();
    void nextDoctor();
    void theatreMenu();
    void transferMenu();
    void billMenu();
    void discharge();
    void undoLast();
    void save();

    void askInsurance(const std::string& patientId);
    void pushUndo(UndoKind kind, const std::string& patientId, const std::string& note);
    std::string opdDepartment();

    std::string dataDir_;
    HospitalMap map_;
    ResourceRouter router_;
    Navigator navigator_;
    PatientRegistry patients_;
    StaffRegistry staff_;
    TriageUnit triage_;
    OTScheduler theatres_;
    TransferDesk transfers_;
    BillingEngine billing_;
    PersistenceManager store_;
    vr_stack undo_;
};

VitalRouteApp::VitalRouteApp(std::string dataDir)
    : dataDir_(std::move(dataDir)), router_(map_), navigator_(map_),
      store_(dataDir_ + "/records")
{
    if (vr_stack_init(&undo_, sizeof(UndoEntry), 8) != 0)
        throw std::bad_alloc();
}

VitalRouteApp::~VitalRouteApp()
{
    vr_stack_free(&undo_);
}

void VitalRouteApp::startUp()
{
    map_.load(dataDir_ + "/hospital_graph.txt");
    router_.loadBeds(dataDir_ + "/resources.txt");
    theatres_.loadTheatres(dataDir_ + "/resources.txt");

    if (store_.hasSavedRecords()) {
        const PersistenceManager::Counts c = store_.load(patients_, staff_, billing_);
        restoreRuntimeState();
        std::cout << "Loaded " << c.patients << " patients, " << c.staff << " staff, "
                  << c.visits << " visits and " << c.bills << " bills from "
                  << store_.directory() << "/\n";
    } else {
        staff_.loadRoster(dataDir_ + "/doctors.txt");
        seed();
        std::cout << "First run: sample patients and staff created (nothing saved yet).\n";
    }
    std::cout << "Floor plan: " << map_.nodeCount() << " landmarks on 3 floors, "
              << router_.freeBeds(CareUnit::ICU) << " ICU and " << router_.freeBeds(CareUnit::Ward)
              << " ward beds free, " << theatres_.theatres().size() << " operation theatres.\n";
}

Patient& VitalRouteApp::admit(const std::string& name, int age, const std::string& phone,
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

void VitalRouteApp::seed()
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

void VitalRouteApp::restoreRuntimeState()
{
    triage_.rebuild(patients_.all());
    for (Patient* p : patients_.all()) {
        if (p->status() != PatientStatus::Admitted || p->bedCode().empty())
            continue;
        try {
            router_.occupy(p->bedCode(), p->id());
        } catch (const std::exception& e) {
            std::cout << "Warning: " << e.what() << '\n';
        }
    }
}

void VitalRouteApp::pushUndo(UndoKind kind, const std::string& patientId, const std::string& note)
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

void VitalRouteApp::askInsurance(const std::string& patientId)
{
    if (!askYes("  Does the patient have insurance?"))
        return;
    const std::string provider = askRequired("  Insurer: ");
    const std::string policy = ask("  Policy number: ");
    const bool flat = choose("Cover type:", {"Flat amount", "Percentage of the bill"}) == 0;
    double value = 0.0, cap = 0.0;
    if (flat) {
        value = askAmount("  Amount covered (Rs): ");
    } else {
        value = askAmount("  Percentage covered (0-100): ");
        cap = askAmount("  Maximum payout in Rs (0 = no limit): ");
    }
    billing_.attachInsurance(patientId, Insurance(provider, policy,
                                                  flat ? Insurance::Kind::Flat
                                                       : Insurance::Kind::Percentage,
                                                  value, cap));
    std::cout << "  Insurance recorded; it is deducted once, when the bill is settled.\n";
}

std::string VitalRouteApp::opdDepartment()
{
    std::vector<std::string> depts;
    for (const std::string& s : staff_.specialties())
        if (s != kEmergencySpecialty)
            depts.push_back(s);
    if (depts.empty())
        return "General Medicine";
    return depts[choose("Department:", depts)];
}

// ---------------------------------------------------------- 1 and 2 --

void VitalRouteApp::registerEmergency()
{
    std::cout << "\n-- Register emergency patient --\n";
    const std::string name = askRequired("  Name: ");
    const int age = askInt("  Age: ", 0, 130);
    const std::string phone = ask("  Phone (optional): ");
    const std::string complaint = askRequired("  Presenting complaint: ");
    long severity = 0;
    parseInt(ask("  Severity, ESI 1 (most critical) to 5: "), severity);
    Patient::checkSeverity(static_cast<int>(severity));   // throws InvalidSeverityException

    Patient& p = admit(name, age, phone, PatientKind::Emergency, static_cast<int>(severity),
                       complaint, kEmergencySpecialty);
    askInsurance(p.id());
    pushUndo(UNDO_REGISTRATION, p.id(), "registration of " + p.name());

    const auto order = triage_.emergencyOrder(patients_);
    const auto pos = std::find(order.begin(), order.end(), &p) - order.begin() + 1;
    std::cout << "Registered " << p.id() << " (ESI " << p.severity() << "). Position in the "
              << "emergency queue: " << pos << " of " << order.size() << ".\n";
}

void VitalRouteApp::registerOpd()
{
    std::cout << "\n-- Register outpatient (OPD) --\n";
    const std::string name = askRequired("  Name: ");
    const int age = askInt("  Age: ", 0, 130);
    const std::string phone = ask("  Phone (optional): ");
    const std::string complaint = askRequired("  Reason for visit: ");
    const std::string dept = opdDepartment();

    Patient& p = admit(name, age, phone, PatientKind::Outpatient, 0, complaint, dept);
    askInsurance(p.id());
    pushUndo(UNDO_REGISTRATION, p.id(), "registration of " + p.name());
    std::cout << "Registered " << p.id() << " for " << dept << ". Position in the OPD line: "
              << triage_.opdCount() << ".\n";
}

// --------------------------------------------------------------- 3 --

void VitalRouteApp::treatNext()
{
    if (triage_.hasEmergency())
        treatEmergency();
    else if (triage_.hasOpd())
        treatOpd();
    else
        std::cout << "Nobody is waiting.\n";
}

void VitalRouteApp::treatEmergency()
{
    const std::string id = triage_.nextEmergency();
    Patient& p = patients_.find(id);
    const int today = vrclock::today();
    std::cout << "\nNext patient: " << p.id() << "  " << p.name() << ", ESI " << p.severity()
              << " - " << p.complaint() << '\n';

    const CareUnit unit = p.requiredUnit();
    BedAssignment bed{};
    if (unit != CareUnit::None) {
        try {
            bed = router_.allocate(p.id(), unit);
        } catch (const std::exception& e) {
            triage_.requeueEmergency(p);     // keeps their place at the front
            std::cout << "Cannot admit yet: " << e.what()
                      << ". The patient stays at the head of the queue.\n";
            return;
        }
    }

    const auto assigned = staff_.assign(kEmergencySpecialty, vrclock::minutesNow(), 30);
    const Doctor* doc = assigned.first;
    const std::string docName = doc ? doc->name() : std::string("duty doctor");
    if (doc) {
        p.setDoctorId(doc->id());
        std::cout << "Attending: " << doc->name() << " from " << vrclock::formatTime(assigned.second)
                  << '\n';
    }

    // Charges accrue as resources are used.
    billing_.charge(p.id(), TestCharge("Emergency panel (ECG, bloods)", today, 1800.0,
                                       p.severity() <= 2));
    billing_.charge(p.id(), ProcedureCharge("Emergency consultation - " + docName, today,
                                            doc ? doc->consultationFee() : 1500.0, 500.0, 250.0));

    if (unit == CareUnit::None) {
        p.setStatus(PatientStatus::Treated);
        p.addVisit(VisitRecord::make(today, docName, "ER (ESI " + std::to_string(p.severity()) +
                                                         "): " + p.complaint(),
                                     "Treated in the ER and released"));
        std::cout << "Treated in the ER and released - ESI " << p.severity()
                  << " does not need a bed.\n";
        return;
    }

    p.assignBed(bed.code, bed.label);
    p.setStatus(PatientStatus::Admitted);
    const double rate = unit == CareUnit::ICU ? kIcuBedPerDay : kWardBedPerDay;
    billing_.charge(p.id(), BedCharge(Patient::unitName(unit) + " bed - " + bed.label, today, rate, 1));
    p.addVisit(VisitRecord::make(today, docName,
                                 "Emergency (ESI " + std::to_string(p.severity()) + "): " +
                                     p.complaint(),
                                 "Admitted to " + bed.label));

    std::cout << "Bed: " << bed.label << " (" << Patient::unitName(unit) << ")\n\n"
              << navigator_.directions(router_.origin(), bed.node, bed.label, bed.side);
}

void VitalRouteApp::treatOpd()
{
    const std::string id = triage_.nextOpd();
    Patient& p = patients_.find(id);
    const int today = vrclock::today();
    std::cout << "\nNext outpatient: " << p.id() << "  " << p.name() << " - " << p.complaint()
              << " (" << p.department() << ")\n";

    auto assigned = staff_.assign(p.department(), vrclock::minutesNow(), 15);
    if (!assigned.first)
        assigned = staff_.assign("General Medicine", vrclock::minutesNow(), 15);
    const Doctor* doc = assigned.first;
    const std::string docName = doc ? doc->name() : std::string("duty doctor");
    if (doc) {
        p.setDoctorId(doc->id());
        std::cout << "Seen by " << doc->name() << " at " << vrclock::formatTime(assigned.second)
                  << '\n';
    }

    std::string diagnosis = ask("  Diagnosis [Routine review]: ");
    if (diagnosis.empty())
        diagnosis = "Routine review";
    std::string prescriptions = ask("  Prescriptions [None]: ");
    if (prescriptions.empty())
        prescriptions = "None";

    billing_.charge(p.id(), ProcedureCharge("OPD consultation - " + docName, today,
                                            doc ? doc->consultationFee() : 600.0, 100.0, 0.0));
    if (askYes("  Dispense medicine from the pharmacy?")) {
        const std::string med = askRequired("    Medicine: ");
        const int qty = askInt("    Quantity: ", 1, 1000);
        const double price = askAmount("    Unit price (Rs): ");
        billing_.charge(p.id(), MedicineCharge(med, today, price, qty));
    }
    p.addVisit(VisitRecord::make(today, docName, diagnosis, prescriptions));
    p.setStatus(PatientStatus::Treated);
    std::cout << "Consultation recorded. Bill so far: "
              << money(billing_.billFor(p.id()).projectedTotal()) << '\n';
}

// ---------------------------------------------------------- 4 and 5 --

void VitalRouteApp::lookUp()
{
    const std::string id = askRequired("Patient ID (e.g. VR-2026-000001): ");
    Patient& p = patients_.find(id);           // hash table: O(1) average

    std::cout << '\n' << p << '\n'
              << "  Complaint : " << p.complaint() << '\n'
              << "  Registered: " << vrclock::formatDate(p.admissionDate()) << '\n';
    if (!p.bedLabel().empty())
        std::cout << "  Bed       : " << p.bedLabel() << '\n';
    if (const Doctor* d = staff_.findDoctor(p.doctorId()))
        std::cout << "  Doctor    : " << d->name() << " (" << d->specialty() << ")\n";
    if (p.dischargeDate())
        std::cout << "  Discharged: " << vrclock::formatDate(p.dischargeDate()) << '\n';
    if (const Bill* b = billing_.find(p.id()))
        std::cout << "  Bill      : " << b->billId() << ", " << money(b->projectedTotal())
                  << (b->settled() ? " (settled)" : " due") << '\n';

    std::cout << "  Visit history, newest first (" << p.visitCount() << "):\n";
    for (const VisitRecord& v : p.visitsNewestFirst())
        std::cout << "    " << vrclock::formatDate(v.date) << "  " << v.doctor << "\n      "
                  << v.diagnosis << "\n      Rx: " << v.prescriptions << '\n';

    const vr_hash_stats st = patients_.hashStats();
    std::cout << "  [hash index: " << st.entries << " entries in " << st.buckets
              << " buckets, load " << std::fixed << std::setprecision(2) << st.load_factor
              << ", " << st.collisions << " collisions, " << st.resizes << " resizes]\n"
              << std::defaultfloat;

    if (askYes("Add a visit note")) {
        const int date = askDate("  Visit date", vrclock::today());
        const std::string doctor = askRequired("  Doctor: ");
        const std::string diagnosis = askRequired("  Diagnosis: ");
        const std::string rx = ask("  Prescriptions: ");
        p.addVisit(VisitRecord::make(date, doctor, diagnosis, rx.empty() ? "None" : rx));
        std::cout << "  Visit added in date order.\n";
    }
}

void VitalRouteApp::admittedBetween()
{
    int from = askDate("From date", vrclock::addDays(vrclock::today(), -30));
    int to = askDate("To date", vrclock::today());
    if (from > to)
        std::swap(from, to);

    const std::vector<Patient*> found = patients_.admittedBetween(from, to);
    std::cout << '\n' << found.size() << " patient(s) registered " << vrclock::formatDate(from)
              << " to " << vrclock::formatDate(to) << ":\n";
    for (const Patient* p : found)
        std::cout << "  " << vrclock::formatDate(p->admissionDate()) << "  " << p->id() << "  "
                  << std::left << std::setw(20) << p->name() << std::right << ' '
                  << std::setw(9) << Patient::kindName(p->kind()) << "  "
                  << Patient::statusName(p->status()) << '\n';
    const vr_avl& idx = patients_.dateIndex();
    std::cout << "  [AVL index: " << idx.nodes << " dates, height " << vr_avl_height(&idx)
              << ", rotations LL " << idx.rot_ll << " RR " << idx.rot_rr << " LR " << idx.rot_lr
              << " RL " << idx.rot_rl << "]\n";
}

// --------------------------------------------------------------- 6 --

void VitalRouteApp::nextDoctor()
{
    const std::vector<std::string> specs = staff_.specialties();
    if (specs.empty()) {
        std::cout << "No doctors on the roster.\n";
        return;
    }
    const std::string spec = specs[choose("Specialty:", specs)];
    const int now = vrclock::minutesNow();
    if (const Doctor* d = staff_.nextAvailable(spec)) {
        std::cout << "\nNext available in " << spec << ": " << d->name() << " - ";
        if (d->nextFree() <= now)
            std::cout << "free now\n";
        else
            std::cout << "free at " << vrclock::formatTime(d->nextFree()) << " (in "
                      << d->nextFree() - now << " min)\n";
    }
    std::cout << "\n  Availability array (sorted by specialty, then next-free time):\n";
    for (const Doctor* d : staff_.availability())
        std::cout << "    " << std::left << std::setw(20) << d->specialty() << std::setw(22)
                  << d->name() << std::right
                  << (d->nextFree() <= now ? std::string("free now")
                                           : "busy until " + vrclock::formatTime(d->nextFree()))
                  << "   seen " << d->patientsSeen() << '\n';
}

// ---------------------------------------------------------- 7 and 8 --

void VitalRouteApp::theatreMenu()
{
    const std::vector<std::string> names = theatres_.theatres();
    if (names.empty()) {
        std::cout << "No operation theatres are configured.\n";
        return;
    }
    const std::size_t pick = choose("Operation theatres:", {"View today's schedule",
                                                            "Book the next free slot",
                                                            "Release a slot", "Back"});
    if (pick == 0) {
        theatres_.printSchedule(std::cout);
    } else if (pick == 1) {
        const std::string theatre = names[choose("Theatre:", names)];
        Patient& p = patients_.find(askRequired("  Patient ID: "));
        const std::string procedure = askRequired("  Procedure: ");
        const double fee = askAmount("  Surgeon's fee (Rs): ");
        const OTScheduler::Booking b = theatres_.book(theatre, p.id(), procedure);
        billing_.charge(p.id(), ProcedureCharge("Surgery: " + procedure + " (" + theatre + ")",
                                                vrclock::today(), fee, 8000.0, 3500.0));
        std::cout << "Booked " << theatre << " slot " << b.slot + 1 << ", "
                  << theatres_.slotTime(theatre, b.slot) << " for " << p.name() << ".\n";
    } else if (pick == 2) {
        const std::string theatre = names[choose("Theatre:", names)];
        const int slot = askInt("  Slot number: ", 1, theatres_.slotCount(theatre)) - 1;
        theatres_.release(theatre, slot);
        std::cout << "Released " << theatre << " slot " << slot + 1
                  << "; it is back in the free-slot ring.\n";
    }
}

void VitalRouteApp::transferMenu()
{
    const std::size_t pick = choose("Ambulance and transfer desk:",
                                    {"New URGENT request (front of the line)",
                                     "New routine request (back of the line)",
                                     "Dispatch the next request", "Cancel the newest routine request",
                                     "List pending requests", "Back"});
    if (pick <= 1) {
        const bool ambulance = choose("Type:", {"Ambulance", "Internal transfer"}) == 0;
        std::string pid = ask("  Patient ID (blank if not registered yet): ");
        if (!pid.empty())
            patients_.find(pid);
        const std::string from = askRequired("  From: ");
        const std::string to = askRequired("  To: ");
        const TransferRequest r = transfers_.request(pick == 0, ambulance, pid, from, to);
        std::cout << "Queued " << TransferDesk::describe(r) << '\n';
    } else if (pick == 2) {
        TransferRequest r;
        if (!transfers_.dispatchNext(r)) {
            std::cout << "No pending requests.\n";
            return;
        }
        std::cout << "Dispatched " << TransferDesk::describe(r) << '\n';
        if (r.ambulance && r.patientId[0]) {
            const Bill* b = billing_.find(r.patientId);
            if (b && !b->settled()) {
                billing_.charge(r.patientId, ProcedureCharge("Ambulance transfer " +
                                                                 std::string(r.code),
                                                             vrclock::today(), 0.0, 2500.0, 300.0));
                std::cout << "Ambulance charge added to " << b->billId() << ".\n";
            }
        }
    } else if (pick == 3) {
        TransferRequest r;
        if (transfers_.cancelLastRoutine(r))
            std::cout << "Cancelled " << TransferDesk::describe(r) << '\n';
        else
            std::cout << "The newest request is not a routine one (or none is pending).\n";
    } else if (pick == 4) {
        const auto pending = transfers_.pending();
        std::cout << pending.size() << " pending (served from the top):\n";
        for (const TransferRequest& r : pending)
            std::cout << "  " << TransferDesk::describe(r) << '\n';
    }
}

// --------------------------------------------------------------- 9 --

void VitalRouteApp::billMenu()
{
    const std::string id = ask("Patient ID (blank to list every bill): ");
    if (id.empty()) {
        const bool byAmount = choose("Sort bills by:", {"Date opened", "Amount"}) == 1;
        const auto bills = billing_.listBills(byAmount ? BillingEngine::SortKey::Amount
                                                       : BillingEngine::SortKey::Date);
        std::cout << '\n';
        for (const Bill* b : bills)
            std::cout << "  " << b->billId() << "  " << b->patientId() << "  "
                      << vrclock::formatDate(b->openedOn()) << "  " << std::setw(14)
                      << money(b->total()) << (b->settled() ? "  settled" : "  open") << '\n';
        std::cout << "  (" << bills.size() << " bills, merge-sorted)\n";
        return;
    }

    const Patient& p = patients_.find(id);
    Bill& bill = billing_.billFor(p.id());
    std::cout << '\n' << bill;
    if (bill.settled())
        return;

    const std::size_t pick = choose("Edit:", {"Add a manual charge (admin only)",
                                              "Record or replace insurance", "Back"});
    if (pick == 0) {
        const Staff* admin = staff_.find(askRequired("  Authorising staff ID: "));
        if (!admin) {
            std::cout << "  No staff member with that ID.\n";
            return;
        }
        const int today = vrclock::today();
        const std::size_t kind = choose("Charge type:", {"Bed", "Medicine", "Test", "Procedure"});
        const std::string desc = askRequired("  Description: ");
        std::unique_ptr<Charge> charge;
        if (kind == 0) {
            const double rate = askAmount("  Rate per day (Rs): ");
            charge.reset(new BedCharge(desc, today, rate, askInt("  Days: ", 1, 365)));
        } else if (kind == 1) {
            const double unit = askAmount("  Unit price (Rs): ");
            charge.reset(new MedicineCharge(desc, today, unit, askInt("  Quantity: ", 1, 10000)));
        } else if (kind == 2) {
            const double price = askAmount("  Price (Rs): ");
            charge.reset(new TestCharge(desc, today, price, askYes("  Urgent (+25%)?")));
        } else {
            const double fee = askAmount("  Professional fee (Rs): ");
            const double facility = askAmount("  Facility fee (Rs): ");
            charge.reset(new ProcedureCharge(desc, today, fee, facility,
                                             askAmount("  Consumables (Rs): ")));
        }
        billing_.manualCharge(p.id(), *charge, *admin);   // throws unless an admin
        pushUndo(UNDO_CHARGE, p.id(), "charge '" + desc + "' on " + bill.billId());
        std::cout << "  Added " << money(charge->calculateCost()) << ". New subtotal "
                  << money(bill.subtotal()) << ".\n";
    } else if (pick == 1) {
        askInsurance(p.id());
    }
}

// -------------------------------------------------------------- 10 --

void VitalRouteApp::discharge()
{
    Patient& p = patients_.find(askRequired("Patient ID: "));
    if (p.status() == PatientStatus::Waiting) {
        std::cout << p.id() << " is still waiting to be seen.\n";
        return;
    }
    if (p.status() == PatientStatus::Discharged) {
        std::cout << p.id() << " was already discharged.\n";
        return;
    }
    Bill& bill = billing_.billFor(p.id());
    int date = askDate("Discharge date", vrclock::today());
    if (date < p.admissionDate()) {
        std::cout << "  Discharge cannot be before admission; using the admission date.\n";
        date = p.admissionDate();
    }

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
        std::cout << "  " << p.bedLabel() << " is free again.\n";
        p.clearBed();
    }
    if (!bill.insurance())
        askInsurance(p.id());

    billing_.settle(p.id(), date);           // insurance applied here, once
    p.setStatus(PatientStatus::Discharged);
    p.setDischargeDate(date);
    p.addVisit(VisitRecord::make(date, "Discharge desk", "Discharged", "Bill " + bill.billId() +
                                                                        " settled"));
    std::cout << '\n' << bill;
}

// -------------------------------------------------------------- 11 --

void VitalRouteApp::undoLast()
{
    UndoEntry e;
    if (vr_stack_pop(&undo_, &e) != 0) {
        std::cout << "Nothing to undo.\n";
        return;
    }
    if (e.kind == UNDO_REGISTRATION) {
        if (!triage_.withdraw(e.patientId)) {
            std::cout << "Cannot undo the " << e.note << ": " << e.patientId
                      << " has already been seen. Entry dropped.\n";
            return;
        }
        billing_.remove(e.patientId);
        patients_.remove(e.patientId);
        std::cout << "Undone: " << e.note << " (" << e.patientId << ").\n";
    } else {
        Bill* b = billing_.find(e.patientId);
        if (!b || b->settled() || b->chargeCount() != e.chargeCount) {
            std::cout << "Cannot undo the " << e.note
                      << ": the bill has changed since (or is settled). Entry dropped.\n";
            return;
        }
        std::unique_ptr<Charge> removed = billing_.undoLastCharge(e.patientId);
        std::cout << "Undone: " << e.note << " (" << money(removed->calculateCost()) << ").\n";
    }
}

// -------------------------------------------------------------- 12 --

void VitalRouteApp::save()
{
    const PersistenceManager::Counts c = store_.save(patients_, staff_, billing_);
    std::cout << "Saved " << c.patients << " patients, " << c.staff << " staff, " << c.visits
              << " visits, " << c.bills << " bills (" << c.charges << " charges) to "
              << store_.directory() << "/\n";
}

void VitalRouteApp::run()
{
    startUp();
    for (;;) {
        std::cout << "\n=========== VitalRoute ===========\n"
                     " 1. Register patient (emergency)\n"
                     " 2. Register patient (OPD)\n"
                     " 3. Treat next patient\n"
                     " 4. Look up patient by ID\n"
                     " 5. Patients admitted between two dates\n"
                     " 6. Find next available doctor\n"
                     " 7. Operation theatre schedule\n"
                     " 8. Ambulance / transfer requests\n"
                     " 9. View or edit a bill\n"
                     "10. Discharge and settle bill\n"
                     "11. Undo last entry\n"
                     "12. Save and exit\n"
                     "==================================\n"
                  << " Waiting: " << triage_.emergencyCount() << " emergency, "
                  << triage_.opdCount() << " OPD";
        if (triage_.hasEmergency())
            std::cout << "   (next: " << triage_.peekEmergency() << ")";
        std::cout << '\n';

        std::string choice;
        try {
            choice = ask("Choose an option: ");
        } catch (const InputClosed&) {
            std::cout << "\nInput closed - exiting without saving.\n";
            return;
        }
        long opt = 0;
        if (!parseInt(choice, opt) || opt < 1 || opt > 12) {
            std::cout << "Please choose 1-12.\n";
            continue;
        }
        try {
            switch (opt) {
            case 1:  registerEmergency(); break;
            case 2:  registerOpd(); break;
            case 3:  treatNext(); break;
            case 4:  lookUp(); break;
            case 5:  admittedBetween(); break;
            case 6:  nextDoctor(); break;
            case 7:  theatreMenu(); break;
            case 8:  transferMenu(); break;
            case 9:  billMenu(); break;
            case 10: discharge(); break;
            case 11: undoLast(); break;
            case 12:
                save();
                std::cout << "Goodbye.\n";
                return;
            }
        } catch (const InputClosed&) {
            std::cout << "\nInput closed - exiting without saving.\n";
            return;
        } catch (const InvalidSeverityException& e) {
            std::cout << "Registration cancelled: " << e.what() << ".\n";
        } catch (const PatientNotFoundException& e) {
            std::cout << "Not found: " << e.what() << ".\n";
        } catch (const BedUnavailableException& e) {
            std::cout << "No bed: " << e.what() << ".\n";
        } catch (const NoRouteFoundException& e) {
            std::cout << "No route: " << e.what() << ".\n";
        } catch (const SlotUnavailableException& e) {
            std::cout << "Theatre: " << e.what() << ".\n";
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << ".\n";
        }
    }
}

std::string findDataDir(int argc, char** argv)
{
    if (argc > 1)
        return argv[1];
    for (const char* dir : {"data", "../data"})
        if (std::ifstream(std::string(dir) + "/hospital_graph.txt"))
            return dir;
    return "data";
}

} // namespace

int main(int argc, char** argv)
{
    try {
        VitalRouteApp app(findDataDir(argc, argv));
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "VitalRoute could not continue: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
