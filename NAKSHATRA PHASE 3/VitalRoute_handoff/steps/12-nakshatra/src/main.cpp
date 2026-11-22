// main.cpp - VitalRoute console.
//
// A thin text front end over HospitalSession (src/app/), which holds all the
// application logic and the undo journal. This file only asks questions,
// calls the session and prints the results behind the twelve-option menu.
#include "HospitalSession.hpp"

#include "Exceptions.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

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

// ------------------------------------------------------------- app --

class VitalRouteApp {
public:
    explicit VitalRouteApp(std::string dataDir) : session_(std::move(dataDir)) {}

    void run();

private:
    void startUp();

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
    std::string opdDepartment();

    HospitalSession session_;
};

void VitalRouteApp::startUp()
{
    const HospitalSession::StartupReport report = session_.startUp();
    if (report.loadedFromRecords) {
        for (const std::string& w : report.warnings)
            std::cout << "Warning: " << w << '\n';
        const PersistenceManager::Counts& c = report.loaded;
        std::cout << "Loaded " << c.patients << " patients, " << c.staff << " staff, "
                  << c.visits << " visits and " << c.bills << " bills from "
                  << session_.recordsDir() << "/\n";
    } else {
        std::cout << "First run: sample patients and staff created (nothing saved yet).\n";
    }
    std::cout << "Floor plan: " << session_.map().nodeCount() << " landmarks on 3 floors, "
              << session_.router().freeBeds(CareUnit::ICU) << " ICU and "
              << session_.router().freeBeds(CareUnit::Ward) << " ward beds free, "
              << session_.theatres().theatres().size() << " operation theatres.\n";
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
    session_.attachInsurance(patientId, Insurance(provider, policy,
                                                  flat ? Insurance::Kind::Flat
                                                       : Insurance::Kind::Percentage,
                                                  value, cap));
    std::cout << "  Insurance recorded; it is deducted once, when the bill is settled.\n";
}

std::string VitalRouteApp::opdDepartment()
{
    const std::vector<std::string> depts = session_.opdDepartments();
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

    // Throws InvalidSeverityException before anything is recorded.
    Patient& p = session_.registerEmergency(name, age, phone, complaint,
                                            static_cast<int>(severity));
    askInsurance(p.id());

    const auto order = session_.emergencyOrder();
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

    Patient& p = session_.registerOpd(name, age, phone, complaint, dept);
    askInsurance(p.id());
    std::cout << "Registered " << p.id() << " for " << dept << ". Position in the OPD line: "
              << session_.triage().opdCount() << ".\n";
}

// --------------------------------------------------------------- 3 --

void VitalRouteApp::treatNext()
{
    if (session_.triage().hasEmergency())
        treatEmergency();
    else if (session_.triage().hasOpd())
        treatOpd();
    else
        std::cout << "Nobody is waiting.\n";
}

void VitalRouteApp::treatEmergency()
{
    using Outcome = HospitalSession::TreatResult::Outcome;
    const HospitalSession::TreatResult r = session_.treatNextEmergency();
    const Patient& p = *r.patient;
    std::cout << "\nNext patient: " << p.id() << "  " << p.name() << ", ESI " << p.severity()
              << " - " << p.complaint() << '\n';

    if (r.outcome == Outcome::NoBed) {
        std::cout << "Cannot admit yet: " << r.error
                  << ". The patient stays at the head of the queue.\n";
        return;
    }
    if (r.doctor)
        std::cout << "Attending: " << r.doctor->name() << " from "
                  << vrclock::formatTime(r.doctorStart) << '\n';

    if (r.outcome == Outcome::ReleasedFromEr) {
        std::cout << "Treated in the ER and released - ESI " << p.severity()
                  << " does not need a bed.\n";
        return;
    }

    std::cout << "Bed: " << r.bed.label << " (" << Patient::unitName(r.bed.unit) << ")\n\n";
    if (!r.routeError.empty())
        std::cout << "No route: " << r.routeError << ".\n";
    else
        std::cout << r.route;
}

void VitalRouteApp::treatOpd()
{
    const HospitalSession::OpdVisit visit = session_.beginOpd();
    const Patient& p = *visit.patient;
    std::cout << "\nNext outpatient: " << p.id() << "  " << p.name() << " - " << p.complaint()
              << " (" << p.department() << ")\n";
    if (visit.doctor)
        std::cout << "Seen by " << visit.doctor->name() << " at "
                  << vrclock::formatTime(visit.doctorStart) << '\n';

    const std::string diagnosis = ask("  Diagnosis [Routine review]: ");
    const std::string prescriptions = ask("  Prescriptions [None]: ");
    std::optional<HospitalSession::MedicineLine> medicine;
    if (askYes("  Dispense medicine from the pharmacy?")) {
        HospitalSession::MedicineLine m;
        m.name = askRequired("    Medicine: ");
        m.quantity = askInt("    Quantity: ", 1, 1000);
        m.unitPrice = askAmount("    Unit price (Rs): ");
        medicine = m;
    }
    const double billSoFar = session_.completeOpd(visit, diagnosis, prescriptions, medicine);
    std::cout << "Consultation recorded. Bill so far: " << money(billSoFar) << '\n';
}

// ---------------------------------------------------------- 4 and 5 --

void VitalRouteApp::lookUp()
{
    const std::string id = askRequired("Patient ID (e.g. VR-2026-000001): ");
    const Patient& p = session_.findPatient(id);   // hash table: O(1) average

    std::cout << '\n' << p << '\n'
              << "  Complaint : " << p.complaint() << '\n'
              << "  Registered: " << vrclock::formatDate(p.admissionDate()) << '\n';
    if (!p.bedLabel().empty())
        std::cout << "  Bed       : " << p.bedLabel() << '\n';
    if (const Doctor* d = session_.findDoctor(p.doctorId()))
        std::cout << "  Doctor    : " << d->name() << " (" << d->specialty() << ")\n";
    if (p.dischargeDate())
        std::cout << "  Discharged: " << vrclock::formatDate(p.dischargeDate()) << '\n';
    if (const Bill* b = session_.findBill(p.id()))
        std::cout << "  Bill      : " << b->billId() << ", " << money(b->projectedTotal())
                  << (b->settled() ? " (settled)" : " due") << '\n';

    std::cout << "  Visit history, newest first (" << p.visitCount() << "):\n";
    for (const VisitRecord& v : p.visitsNewestFirst())
        std::cout << "    " << vrclock::formatDate(v.date) << "  " << v.doctor << "\n      "
                  << v.diagnosis << "\n      Rx: " << v.prescriptions << '\n';

    const vr_hash_stats st = session_.patients().hashStats();
    std::cout << "  [hash index: " << st.entries << " entries in " << st.buckets
              << " buckets, load " << std::fixed << std::setprecision(2) << st.load_factor
              << ", " << st.collisions << " collisions, " << st.resizes << " resizes]\n"
              << std::defaultfloat;

    if (askYes("Add a visit note")) {
        const int date = askDate("  Visit date", vrclock::today());
        const std::string doctor = askRequired("  Doctor: ");
        const std::string diagnosis = askRequired("  Diagnosis: ");
        const std::string rx = ask("  Prescriptions: ");
        session_.addVisitNote(p.id(), date, doctor, diagnosis, rx);
        std::cout << "  Visit added in date order.\n";
    }
}

void VitalRouteApp::admittedBetween()
{
    int from = askDate("From date", vrclock::addDays(vrclock::today(), -30));
    int to = askDate("To date", vrclock::today());
    if (from > to)
        std::swap(from, to);

    const std::vector<Patient*> found = session_.admittedBetween(from, to);
    std::cout << '\n' << found.size() << " patient(s) registered " << vrclock::formatDate(from)
              << " to " << vrclock::formatDate(to) << ":\n";
    for (const Patient* p : found)
        std::cout << "  " << vrclock::formatDate(p->admissionDate()) << "  " << p->id() << "  "
                  << std::left << std::setw(20) << p->name() << std::right << ' '
                  << std::setw(9) << Patient::kindName(p->kind()) << "  "
                  << Patient::statusName(p->status()) << '\n';
    const vr_avl& idx = session_.patients().dateIndex();
    std::cout << "  [AVL index: " << idx.nodes << " dates, height " << vr_avl_height(&idx)
              << ", rotations LL " << idx.rot_ll << " RR " << idx.rot_rr << " LR " << idx.rot_lr
              << " RL " << idx.rot_rl << "]\n";
}

// --------------------------------------------------------------- 6 --

void VitalRouteApp::nextDoctor()
{
    const std::vector<std::string> specs = session_.specialties();
    if (specs.empty()) {
        std::cout << "No doctors on the roster.\n";
        return;
    }
    const std::string spec = specs[choose("Specialty:", specs)];
    const int now = vrclock::minutesNow();
    if (const Doctor* d = session_.nextAvailableDoctor(spec)) {
        std::cout << "\nNext available in " << spec << ": " << d->name() << " - ";
        if (d->nextFree() <= now)
            std::cout << "free now\n";
        else
            std::cout << "free at " << vrclock::formatTime(d->nextFree()) << " (in "
                      << d->nextFree() - now << " min)\n";
    }
    std::cout << "\n  Availability array (sorted by specialty, then next-free time):\n";
    for (const Doctor* d : session_.staff().availability())
        std::cout << "    " << std::left << std::setw(20) << d->specialty() << std::setw(22)
                  << d->name() << std::right
                  << (d->nextFree() <= now ? std::string("free now")
                                           : "busy until " + vrclock::formatTime(d->nextFree()))
                  << "   seen " << d->patientsSeen() << '\n';
}

// ---------------------------------------------------------- 7 and 8 --

void VitalRouteApp::theatreMenu()
{
    const OTScheduler& ot = session_.theatres();
    const std::vector<std::string> names = ot.theatres();
    if (names.empty()) {
        std::cout << "No operation theatres are configured.\n";
        return;
    }
    const std::size_t pick = choose("Operation theatres:", {"View today's schedule",
                                                            "Book the next free slot",
                                                            "Release a slot", "Back"});
    if (pick == 0) {
        ot.printSchedule(std::cout);
    } else if (pick == 1) {
        const std::string theatre = names[choose("Theatre:", names)];
        const Patient& p = session_.findPatient(askRequired("  Patient ID: "));
        const std::string procedure = askRequired("  Procedure: ");
        const double fee = askAmount("  Surgeon's fee (Rs): ");
        const OTScheduler::Booking b = session_.bookTheatre(theatre, p.id(), procedure, fee);
        std::cout << "Booked " << theatre << " slot " << b.slot + 1 << ", "
                  << ot.slotTime(theatre, b.slot) << " for " << p.name() << ".\n";
    } else if (pick == 2) {
        const std::string theatre = names[choose("Theatre:", names)];
        const int slot = askInt("  Slot number: ", 1, ot.slotCount(theatre)) - 1;
        session_.releaseTheatreSlot(theatre, slot);
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
            session_.findPatient(pid);   // fail before asking anything else
        const std::string from = askRequired("  From: ");
        const std::string to = askRequired("  To: ");
        const TransferRequest r = session_.requestTransfer(pick == 0, ambulance, pid, from, to);
        std::cout << "Queued " << TransferDesk::describe(r) << '\n';
    } else if (pick == 2) {
        const HospitalSession::DispatchResult d = session_.dispatchNextTransfer();
        if (!d.dispatched) {
            std::cout << "No pending requests.\n";
            return;
        }
        std::cout << "Dispatched " << TransferDesk::describe(d.request) << '\n';
        if (!d.chargedBillId.empty())
            std::cout << "Ambulance charge added to " << d.chargedBillId << ".\n";
    } else if (pick == 3) {
        TransferRequest r;
        if (session_.cancelLastRoutineTransfer(r))
            std::cout << "Cancelled " << TransferDesk::describe(r) << '\n';
        else
            std::cout << "The newest request is not a routine one (or none is pending).\n";
    } else if (pick == 4) {
        const auto pending = session_.transfers().pending();
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
        const auto bills = session_.listBills(byAmount);
        std::cout << '\n';
        for (const Bill* b : bills)
            std::cout << "  " << b->billId() << "  " << b->patientId() << "  "
                      << vrclock::formatDate(b->openedOn()) << "  " << std::setw(14)
                      << money(b->total()) << (b->settled() ? "  settled" : "  open") << '\n';
        std::cout << "  (" << bills.size() << " bills, merge-sorted)\n";
        return;
    }

    const Patient& p = session_.findPatient(id);
    const Bill& bill = session_.billFor(p.id());
    std::cout << '\n' << bill;
    if (bill.settled())
        return;

    const std::size_t pick = choose("Edit:", {"Add a manual charge (admin only)",
                                              "Record or replace insurance", "Back"});
    if (pick == 0) {
        const std::string staffId = askRequired("  Authorising staff ID: ");
        if (!session_.findStaff(staffId)) {
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
        session_.addManualCharge(p.id(), *charge, staffId);   // throws unless an admin
        std::cout << "  Added " << money(charge->calculateCost()) << ". New subtotal "
                  << money(bill.subtotal()) << ".\n";
    } else if (pick == 1) {
        askInsurance(p.id());
    }
}

// -------------------------------------------------------------- 10 --

void VitalRouteApp::discharge()
{
    using Block = HospitalSession::DischargeBlock;
    const Patient& p = session_.findPatient(askRequired("Patient ID: "));
    const Block block = session_.dischargeBlocker(p.id());
    if (block == Block::StillWaiting) {
        std::cout << p.id() << " is still waiting to be seen.\n";
        return;
    }
    if (block == Block::AlreadyDischarged) {
        std::cout << p.id() << " was already discharged.\n";
        return;
    }
    const Bill& bill = session_.billFor(p.id());
    const int requested = askDate("Discharge date", vrclock::today());

    const HospitalSession::BedRelease r = session_.prepareDischarge(p.id(), requested);
    if (r.dateClamped)
        std::cout << "  Discharge cannot be before admission; using the admission date.\n";
    if (!r.releasedBedLabel.empty())
        std::cout << "  " << r.releasedBedLabel << " is free again.\n";
    if (!bill.insurance())
        askInsurance(p.id());

    session_.settleDischarge(p.id(), r.date);   // insurance applied here, once
    std::cout << '\n' << bill;
}

// -------------------------------------------------------------- 11 --

void VitalRouteApp::undoLast()
{
    using Kind = HospitalSession::UndoResult::Kind;
    const HospitalSession::UndoResult r = session_.undoLast();
    switch (r.kind) {
    case Kind::Nothing:
        std::cout << "Nothing to undo.\n";
        break;
    case Kind::RegistrationDropped:
        std::cout << "Cannot undo the " << r.note << ": " << r.patientId
                  << " has already been seen. Entry dropped.\n";
        break;
    case Kind::RegistrationUndone:
        std::cout << "Undone: " << r.note << " (" << r.patientId << ").\n";
        break;
    case Kind::ChargeDropped:
        std::cout << "Cannot undo the " << r.note
                  << ": the bill has changed since (or is settled). Entry dropped.\n";
        break;
    case Kind::ChargeUndone:
        std::cout << "Undone: " << r.note << " (" << money(r.amount) << ").\n";
        break;
    }
}

// -------------------------------------------------------------- 12 --

void VitalRouteApp::save()
{
    const PersistenceManager::Counts c = session_.save();
    std::cout << "Saved " << c.patients << " patients, " << c.staff << " staff, " << c.visits
              << " visits, " << c.bills << " bills (" << c.charges << " charges) to "
              << session_.recordsDir() << "/\n";
}

void VitalRouteApp::run()
{
    startUp();
    for (;;) {
        const TriageUnit& triage = session_.triage();
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
                  << " Waiting: " << triage.emergencyCount() << " emergency, "
                  << triage.opdCount() << " OPD";
        if (triage.hasEmergency())
            std::cout << "   (next: " << triage.peekEmergency() << ")";
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
