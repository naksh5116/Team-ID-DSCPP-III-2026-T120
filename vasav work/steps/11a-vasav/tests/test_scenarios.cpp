// test_scenarios.cpp - scenario and integration tests for the C++ layers.
//
// Required tests T5-T8 run first, then extra coverage (X6-X9). Tests use
// the real floor plan and resources under data/, and write records to
// build/test_records/. Run from the repository root (run_tests.sh does).
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

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failedChecks = 0;
std::string g_data = "data";

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            ++g_failedChecks;                                                         \
            std::cout << "       check failed: " #cond "  (" << __FILE__ << ':'      \
                      << __LINE__ << ")\n";                                           \
        }                                                                             \
    } while (0)

#define CHECK_THROWS(ExceptionType, expr)                                             \
    do {                                                                              \
        bool thrown_ = false;                                                         \
        try {                                                                         \
            expr;                                                                     \
        } catch (const ExceptionType&) {                                              \
            thrown_ = true;                                                           \
        }                                                                             \
        if (!thrown_) {                                                               \
            ++g_failedChecks;                                                         \
            std::cout << "       expected " #ExceptionType " from: " #expr "  ("     \
                      << __FILE__ << ':' << __LINE__ << ")\n";                        \
        }                                                                             \
    } while (0)

bool near(double a, double b)
{
    return std::fabs(a - b) < 0.005;
}

bool run(const std::string& label, const std::function<void()>& test)
{
    const int before = g_failedChecks;
    try {
        test();
    } catch (const std::exception& e) {
        ++g_failedChecks;
        std::cout << "       unexpected exception: " << e.what() << '\n';
    }
    const bool ok = g_failedChecks == before;
    std::cout << '[' << (ok ? "PASS" : "FAIL") << "] " << label << '\n';
    return ok;
}

const int kDay = 20260915;   // fixed dates keep the tests repeatable

Patient& emergency(PatientRegistry& reg, TriageUnit& triage, const std::string& name,
                   int severity, int date = kDay)
{
    Patient& p = reg.registerPatient(name, 40, "", PatientKind::Emergency, severity, date,
                                     "test complaint", "Emergency Medicine");
    triage.admitEmergency(p);
    return p;
}

void loadHospital(HospitalMap& map, ResourceRouter& router)
{
    map.load(g_data + "/hospital_graph.txt");
    router.loadBeds(g_data + "/resources.txt");
}

// --------------------------------------------------------------- T5 --

void testTriageAndRouting()
{
    HospitalMap map;
    ResourceRouter router(map);
    loadHospital(map, router);
    PatientRegistry reg;
    TriageUnit triage;

    // Arrival order with known severities.
    Patient& a = emergency(reg, triage, "A (ESI 3)", 3);
    Patient& b = emergency(reg, triage, "B (ESI 1)", 1);
    Patient& c = emergency(reg, triage, "C (ESI 4)", 4);
    Patient& d = emergency(reg, triage, "D (ESI 1)", 1);
    Patient& e = emergency(reg, triage, "E (ESI 2)", 2);
    Patient& f = emergency(reg, triage, "F (ESI 5)", 5);

    const std::vector<const Patient*> expectedOrder = {&b, &d, &e, &a, &c, &f};
    // Patient::operator< agrees with the heap.
    CHECK(triage.emergencyOrder(reg) == expectedOrder);
    CHECK(c < a && a < e && e < b && d < b);   // d arrived after b: less urgent

    const std::vector<std::string> expectedBeds = {"ICU1-01", "ICU1-02", "ICU1-03",
                                                   "AMU-01", "", ""};
    const int amuNode = map.findByName("Acute Medical Unit");
    const std::vector<double> dist = map.distancesFrom(router.origin());
    for (std::size_t i = 0; i < expectedOrder.size(); ++i) {
        const std::string id = triage.nextEmergency();
        CHECK(id == expectedOrder[i]->id());
        const Patient& p = reg.find(id);
        if (p.requiredUnit() == CareUnit::None) {
            CHECK(expectedBeds[i].empty());
            continue;
        }
        const BedAssignment bed = router.allocate(p.id(), p.requiredUnit());
        CHECK(bed.code == expectedBeds[i]);
        CHECK(bed.unit == p.requiredUnit());
        if (p.requiredUnit() == CareUnit::ICU) {
            // Regression for the type-then-distance ordering: a general bed
            // in the AMU is much nearer, yet the ICU patient gets an ICU bed.
            CHECK(dist[static_cast<std::size_t>(amuNode)] < bed.distance);
            CHECK(router.freeBeds(CareUnit::Ward) > 0);
        }
    }
    CHECK(!triage.hasEmergency());
    CHECK(router.findBed("ICU1-01")->occupant == b.id());

    // ICU-1 fills up, so the next ICU patient goes to the further ICU-2.
    Patient& g = emergency(reg, triage, "G (ESI 1)", 1);
    Patient& h = emergency(reg, triage, "H (ESI 2)", 2);
    CHECK(router.allocate(triage.nextEmergency(), g.requiredUnit()).code == "ICU1-04");
    const BedAssignment far = router.allocate(triage.nextEmergency(), h.requiredUnit());
    CHECK(far.code == "ICU2-05" && far.label == "ICU-2, Bed 5");
    CHECK(near(far.distance, 116.0));

    // Remaining ICU beds go, then there is none - even with ward beds free.
    for (int i = 0; i < 3; ++i)
        router.allocate("filler", CareUnit::ICU);
    CHECK_THROWS(BedUnavailableException, router.allocate("late", CareUnit::ICU));
    CHECK(router.freeBeds(CareUnit::Ward) == 21);

    // Severity outside 1-5 never reaches the queue.
    CHECK_THROWS(InvalidSeverityException, emergency(reg, triage, "Bad", 0));
    CHECK_THROWS(InvalidSeverityException, emergency(reg, triage, "Bad", 6));
    CHECK(!triage.hasEmergency());
}

// --------------------------------------------------------------- T6 --

void testNavigation()
{
    HospitalMap map;
    ResourceRouter router(map);
    loadHospital(map, router);
    Navigator nav(map);

    const Bed* bed7 = router.findBed("ICU2-07");
    CHECK(bed7 != nullptr);
    const Route r = nav.directions(router.origin(), bed7->node, router.labelOf(*bed7), bed7->side);
    const std::vector<std::string> expected = {
        "From the Central Emergency Room, head towards ER Triage Desk",
        "Walk 34 m along the Reception Corridor",
        "Turn left at Main Corridor Junction",
        "Walk 12 m",
        "Bear slightly left at Pharmacy Corner",
        "Walk 9 m",
        "Take the lift to floor 1",
        "Exit the lift heading south towards Nursing Station 1A",
        "Walk 10 m",
        "Turn right at Nursing Station 1A",
        "Walk 12 m along the West Wing Corridor",
        "Bear slightly left at the West Wing Corridor",
        "Walk 12 m",
        "Turn left at Critical Care Bend",
        "Walk 12 m",
        "Arrive at ICU-2, Bed 7 - on your left",
    };
    CHECK(r.steps == expected);
    if (r.steps != expected)
        for (const std::string& s : r.steps)
            std::cout << "       got: " << s << '\n';
    CHECK(near(r.distance, 101.0));   // the lift ride is not walked
    CHECK(r.seconds == 84);           // 101 m at 1.2 m/s

    std::ostringstream printed;
    printed << r;
    CHECK(printed.str().find("ROUTE: Central Emergency Room  ->  ICU-2, Bed 7") == 0);
    CHECK(printed.str().find("Total distance: 101 m") != std::string::npos);

    // Stairs are cheaper than a lift to reach General Ward B; two flights
    // merge into one instruction and the straight run merges too.
    const Route stairs = nav.directions(router.origin(), map.findByName("General Ward B"));
    const std::vector<std::string> expectedStairs = {
        "From the Central Emergency Room, head towards ER Triage Desk",
        "Walk 48 m along the Reception Corridor",
        "Turn left at the East Corridor",
        "Walk 14 m",
        "Take the stairs to floor 2",
        "Leave the stairwell heading south towards Nursing Station 2B",
        "Walk 22 m",
        "Arrive at General Ward B",
    };
    CHECK(stairs.steps == expectedStairs);
    CHECK(near(stairs.distance, 120.0));

    // Turn classification thresholds; positive = left.
    CHECK(Navigator::classifyTurn(19.9) == "Continue straight");
    CHECK(Navigator::classifyTurn(-20.0) == "Bear slightly right");
    CHECK(Navigator::classifyTurn(59.0) == "Bear slightly left");
    CHECK(Navigator::classifyTurn(90.0) == "Turn left");
    CHECK(Navigator::classifyTurn(-135.0) == "Turn right");
    CHECK(Navigator::classifyTurn(170.0) == "Turn sharply left");
    CHECK(near(Navigator::normalise(270.0), -90.0));
    CHECK(near(Navigator::normalise(-180.0), 180.0));

    // Unreachable destination.
    HospitalMap island;
    island.addNode(0, 0, 0, 0, NODE_ER, "ER");
    island.addNode(1, 0, 10, 0, NODE_WARD, "Cut-off Ward");
    Navigator lost(island);
    CHECK_THROWS(NoRouteFoundException, lost.directions(0, 1));
}

// --------------------------------------------------------------- T7 --

void testBillingWithInsurance()
{
    Bill bill("BL-900001", "VR-2026-000001", kDay);
    bill += BedCharge("ICU bed - ICU-1, Bed 2", kDay, 4500.0, 2);          //  9000.00
    bill += MedicineCharge("Ceftriaxone 1 g", kDay, 35.50, 10);           //   355.00
    bill += TestCharge("CT head", kDay, 1200.0, true);                    //  1500.00
    bill += ProcedureCharge("Appendectomy", kDay, 15000.0, 6000.0, 2500.0); // 23500.00
    // subtotal 34355.00; 20% insurance = 6871.00; patient pays 27484.00
    CHECK(bill.chargeCount() == 4);
    CHECK(near(bill.subtotal(), 34355.0));
    CHECK(near(bill.total(), 34355.0));             // nothing deducted yet

    bill.setInsurance(Insurance("Star Health", "SH-1", Insurance::Kind::Percentage, 20.0));
    CHECK(near(bill.insuranceDeduction(), 0.0));    // only at settlement
    CHECK(near(bill.projectedTotal(), 27484.0));
    bill.settle(kDay + 3);
    CHECK(near(bill.insuranceDeduction(), 6871.0));
    CHECK(near(bill.total(), 27484.0));
    CHECK_THROWS(std::logic_error, bill.settle(kDay + 4));             // applied once
    CHECK_THROWS(std::logic_error, bill += TestCharge("late", kDay, 1.0, false));

    std::ostringstream printed;
    printed << bill;
    CHECK(printed.str().find("27484.00") != std::string::npos);
    CHECK(printed.str().find("TOTAL PAID") != std::string::npos);

    // A cap limits a percentage; a flat amount never takes the bill below zero.
    Bill capped("BL-900002", "VR-2026-000002", kDay);
    capped += ProcedureCharge("Appendectomy", kDay, 15000.0, 6000.0, 2500.0);
    capped.setInsurance(Insurance("Capped", "", Insurance::Kind::Percentage, 20.0, 1000.0));
    capped.settle(kDay);
    CHECK(near(capped.total(), 22500.0));
    Bill tiny("BL-900003", "VR-2026-000003", kDay);
    tiny += MedicineCharge("Paracetamol 500 mg", kDay, 2.0, 10);
    tiny.setInsurance(Insurance("Generous", "", Insurance::Kind::Flat, 5000.0));
    tiny.settle(kDay);
    CHECK(near(tiny.insuranceDeduction(), 20.0) && near(tiny.total(), 0.0));
    CHECK_THROWS(std::invalid_argument,
                 Insurance("Bad", "", Insurance::Kind::Percentage, 120.0));

    // The engine: bill += charge, admin-only manual entries, undo, sorting.
    BillingEngine engine;
    engine.open("VR-2026-000010", kDay + 1);
    engine.open("VR-2026-000011", kDay);
    engine.charge("VR-2026-000010", TestCharge("X-ray", kDay, 400.0, false));
    engine.charge("VR-2026-000011", BedCharge("Ward bed", kDay, 2500.0, 3));
    Admin admin("AD-9", "Test Admin", 40, "", "Administration", "Day", "Office");
    Nurse nurse("NR-9", "Test Nurse", 30, "", "Ward", "Day", "Ward A");
    CHECK_THROWS(std::runtime_error,
                 engine.manualCharge("VR-2026-000010", MedicineCharge("Gauze", kDay, 5.0, 2), nurse));
    engine.manualCharge("VR-2026-000010", MedicineCharge("Gauze", kDay, 5.0, 2), admin);
    CHECK(near(engine.billFor("VR-2026-000010").subtotal(), 410.0));
    std::unique_ptr<Charge> undone = engine.undoLastCharge("VR-2026-000010");
    CHECK(undone && undone->description() == "Gauze");
    CHECK(near(engine.billFor("VR-2026-000010").subtotal(), 400.0));
    const auto byDate = engine.listBills(BillingEngine::SortKey::Date);
    CHECK(byDate.size() == 2 && byDate[0]->patientId() == "VR-2026-000011");
    const auto byAmount = engine.listBills(BillingEngine::SortKey::Amount);
    CHECK(byAmount[0]->patientId() == "VR-2026-000011");     // 7500 before 400
    CHECK_THROWS(PatientNotFoundException, engine.billFor("VR-2026-999999"));
}

// --------------------------------------------------------------- T8 --

void testFullJourney()
{
    const std::string dir = "build/test_records";
    for (const char* f : {"patients.dat", "staff.dat", "visits.dat", "bills.dat"})
        std::remove((dir + "/" + f).c_str());

    HospitalMap map;
    ResourceRouter router(map);
    loadHospital(map, router);
    Navigator nav(map);
    PatientRegistry reg;
    StaffRegistry staff;
    staff.loadRoster(g_data + "/doctors.txt");
    TriageUnit triage;
    BillingEngine billing;

    // Register.
    Patient& p = reg.registerPatient("Journey Patient", 52, "98000-00000", PatientKind::Emergency,
                                     2, kDay, "Chest pain | sweating", "Emergency Medicine");
    billing.open(p.id(), kDay);
    billing.attachInsurance(p.id(), Insurance("CGHS", "CG-1", Insurance::Kind::Flat, 1000.0));
    triage.admitEmergency(p);
    Patient& waiting = reg.registerPatient("Still Waiting", 30, "", PatientKind::Outpatient, 0,
                                           kDay + 1, "Rash", "General Medicine");
    triage.joinOpd(waiting);
    billing.open(waiting.id(), kDay + 1);

    // Triage and route.
    CHECK(triage.nextEmergency() == p.id());
    const BedAssignment bed = router.allocate(p.id(), p.requiredUnit());
    CHECK(bed.code == "ICU1-01");
    p.assignBed(bed.code, bed.label);
    p.setStatus(PatientStatus::Admitted);
    const Route route = nav.directions(router.origin(), bed.node, bed.label, bed.side);
    CHECK(route.steps.back() == "Arrive at ICU-1, Bed 1 - on your left");

    // Consult.
    const auto seen = staff.assign("Emergency Medicine", 600, 30);
    CHECK(seen.first && seen.first->id() == "DR-01" && seen.second == 600);
    p.setDoctorId(seen.first->id());
    p.addVisit(VisitRecord::make(kDay, seen.first->name(), "Unstable angina", "Aspirin; GTN"));

    // Bill: consultation + 3 ICU days (1 on admission, 2 more at discharge).
    billing.charge(p.id(), ProcedureCharge("Emergency consultation", kDay,
                                           seen.first->consultationFee(), 500.0, 250.0));
    billing.charge(p.id(), BedCharge("ICU bed - " + bed.label, kDay, 6000.0, 1));
    const int leaving = vrclock::addDays(kDay, 3);
    billing.charge(p.id(), BedCharge("ICU bed - further days", leaving, 6000.0,
                                     vrclock::daysBetween(kDay, leaving) - 1));

    // Discharge.
    router.release(p.bedCode());
    p.clearBed();
    const Bill& settled = billing.settle(p.id(), leaving);
    CHECK(near(settled.subtotal(), 1500.0 + 750.0 + 18000.0));
    CHECK(near(settled.total(), 20250.0 - 1000.0));
    p.setStatus(PatientStatus::Discharged);
    p.setDischargeDate(leaving);
    p.addVisit(VisitRecord::make(leaving, "Discharge desk", "Discharged", "Bill settled"));

    // Save, then reload into brand-new stores.
    PersistenceManager store(dir);
    const PersistenceManager::Counts saved = store.save(reg, staff, billing);
    CHECK(saved.patients == 2 && saved.visits == 2 && saved.bills == 2 && saved.charges == 3);
    CHECK(store.hasSavedRecords());

    PatientRegistry reg2;
    StaffRegistry staff2;
    BillingEngine billing2;
    const PersistenceManager::Counts loaded = store.load(reg2, staff2, billing2);
    CHECK(loaded.patients == 2 && loaded.staff == staff.size() && loaded.charges == 3);

    const Patient& q = reg2.find(p.id());          // hash index rebuilt
    CHECK(q.name() == p.name() && q.age() == 52 && q.phone() == p.phone());
    CHECK(q.complaint() == "Chest pain | sweating");   // escaped separator survives
    CHECK(q.severity() == 2 && q.status() == PatientStatus::Discharged);
    CHECK(q.admissionDate() == kDay && q.dischargeDate() == leaving);
    CHECK(q.doctorId() == "DR-01" && q.bedCode().empty());
    const auto before = p.visitsOldestFirst();
    const auto after = q.visitsOldestFirst();
    CHECK(after.size() == before.size());
    for (std::size_t i = 0; i < after.size() && i < before.size(); ++i)
        CHECK(after[i].date == before[i].date &&
              std::string(after[i].diagnosis) == before[i].diagnosis &&
              std::string(after[i].prescriptions) == before[i].prescriptions);

    const Bill* b2 = billing2.find(p.id());
    CHECK(b2 && b2->billId() == settled.billId() && b2->settled());
    CHECK(b2 && b2->chargeCount() == 3 && near(b2->subtotal(), settled.subtotal()));
    CHECK(b2 && near(b2->total(), settled.total()) && b2->settledOn() == leaving);
    CHECK(b2 && b2->insurance() && b2->insurance()->provider() == "CGHS");

    // AVL index rebuilt: both registrations are in range, in date order.
    const auto inRange = reg2.admittedBetween(kDay, kDay + 1);
    CHECK(inRange.size() == 2 && inRange[0]->id() == p.id());
    CHECK(reg2.admittedBetween(kDay + 2, kDay + 30).empty());

    // Runtime state comes back: the doctor's diary and the OPD line.
    CHECK(staff2.findDoctor("DR-01")->nextFree() == 630);
    CHECK(staff2.nextAvailable("Emergency Medicine")->id() == "DR-02");
    TriageUnit triage2;
    triage2.rebuild(reg2.all());
    CHECK(triage2.nextOpd() == waiting.id());

    // New registrations continue the numbering.
    CHECK(reg2.registerPatient("Next", 20, "", PatientKind::Outpatient, 0, kDay, "x",
                               "General Medicine").id() == "VR-2026-000003");
}

// ------------------------------------------------------------ extras --

void testTheatreSlots()
{
    OTScheduler ot;
    ot.loadTheatres(g_data + "/resources.txt");
    CHECK(ot.theatres().size() == 2 && ot.slotCount("OT-1") == 8);
    for (int i = 0; i < 8; ++i)
        CHECK(ot.book("OT-1", "VR-2026-00000" + std::to_string(i + 1), "Procedure").slot == i);
    CHECK(ot.freeSlots("OT-1") == 0);
    CHECK_THROWS(SlotUnavailableException, ot.book("OT-1", "VR-2026-000009", "Late"));
    ot.release("OT-1", 3);                              // a middle slot
    CHECK(ot.occupant("OT-1", 2) == "VR-2026-000003");  // neighbours untouched
    CHECK(ot.occupant("OT-1", 4) == "VR-2026-000005");
    const OTScheduler::Booking again = ot.book("OT-1", "VR-2026-000009", "Late");
    CHECK(again.slot == 3 && again.startMinute == 11 * 60);
    CHECK_THROWS(SlotUnavailableException, ot.release("OT-2", 0));   // not booked
    CHECK_THROWS(SlotUnavailableException, ot.book("OT-9", "x", "y"));
}

void testTransferDesk()
{
    TransferDesk desk;
    desk.request(false, true, "", "City Clinic", "ER");          // R1
    desk.request(false, false, "", "AMU", "Radiology");          // R2
    desk.request(true, true, "", "Highway", "ER");               // U1
    desk.request(false, false, "", "Ward B", "OT-1");            // R3
    desk.request(true, false, "", "ICU-1", "OT-2");              // U2
    const auto pending = desk.pending();
    const std::vector<std::string> order = {"TR-0005", "TR-0003", "TR-0001", "TR-0002", "TR-0004"};
    CHECK(pending.size() == order.size());
    for (std::size_t i = 0; i < pending.size() && i < order.size(); ++i)
        CHECK(pending[i].code == order[i]);

    TransferRequest r;
    CHECK(desk.cancelLastRoutine(r) && std::string(r.code) == "TR-0004");
    CHECK(desk.dispatchNext(r) && std::string(r.code) == "TR-0005" && r.urgent);
    desk.request(true, false, "", "ER", "ICU-2");                 // jumps to the front
    CHECK(desk.dispatchNext(r) && std::string(r.code) == "TR-0006");
    while (desk.dispatchNext(r)) {
    }
    CHECK(desk.size() == 0 && !desk.cancelLastRoutine(r));
}

void testNextAvailableDoctor()
{
    StaffRegistry staff;
    staff.loadRoster(g_data + "/doctors.txt");
    CHECK(staff.doctors().size() == 6 && staff.nurses().size() == 2 && staff.admins().size() == 1);
    CHECK(staff.nextAvailable("Emergency Medicine")->id() == "DR-01");
    CHECK(staff.assign("Emergency Medicine", 600, 30).first->id() == "DR-01");   // 10:00-10:30
    CHECK(staff.assign("Emergency Medicine", 600, 30).first->id() == "DR-02");   // 10:00-10:30
    const auto third = staff.assign("Emergency Medicine", 605, 30);              // both busy
    CHECK(third.first->id() == "DR-01" && third.second == 630);
    CHECK(staff.nextAvailable("Emergency Medicine")->id() == "DR-02");
    CHECK(staff.nextAvailable("Dermatology") == nullptr);
    CHECK(staff.assign("Dermatology", 600, 10).first == nullptr);
    // The availability array stays sorted by (specialty, next-free time).
    const auto avail = staff.availability();
    for (std::size_t i = 1; i < avail.size(); ++i)
        if (avail[i - 1]->specialty() == avail[i]->specialty())
            CHECK(avail[i - 1]->nextFree() <= avail[i]->nextFree());
    CHECK(staff.find("AD-01")->canAuthorizeCharges() && !staff.find("NR-01")->canAuthorizeCharges());
    CHECK(staff.find("DR-03")->canPrescribe() && !staff.find("AD-01")->canPrescribe());
}

void testRegistryAndUndo()
{
    PatientRegistry reg;
    TriageUnit triage;
    Patient& a = emergency(reg, triage, "Alpha", 3, 20260101);
    Patient& b = emergency(reg, triage, "Bravo", 1, 20260301);
    Patient& c = reg.registerPatient("Charlie", 5, "", PatientKind::Outpatient, 0, 20260201,
                                     "Cough", "Paediatrics");
    triage.joinOpd(c);
    CHECK(a.id() == "VR-2026-000001" && reg.size() == 3);
    CHECK(&reg.find(b.id()) == &b);
    CHECK_THROWS(PatientNotFoundException, reg.find("VR-2026-424242"));

    // Undo a registration: out of the heap, the hash and the AVL index.
    CHECK(triage.withdraw(b.id()));
    const std::string bId = b.id();
    reg.remove(bId);
    CHECK(!reg.contains(bId) && reg.size() == 2);
    CHECK(reg.admittedBetween(20260301, 20260331).empty());
    CHECK(triage.nextEmergency() == a.id());
    CHECK(triage.withdraw(c.id()) && !triage.hasOpd());
    CHECK(!triage.withdraw(c.id()));

    // Visits stay in date order however they arrive.
    c.addVisit(VisitRecord::make(20260210, "Dr. B", "second", ""));
    c.addVisit(VisitRecord::make(20260205, "Dr. A", "first", ""));     // head
    c.addVisit(VisitRecord::make(20260220, "Dr. D", "fourth", ""));    // tail
    c.addVisit(VisitRecord::make(20260215, "Dr. C", "third", ""));     // middle
    const auto newest = c.visitsNewestFirst();
    CHECK(newest.size() == 4 && std::string(newest[0].diagnosis) == "fourth" &&
          std::string(newest[3].diagnosis) == "first");
    Patient copy = c;                                                  // deep copy
    c.addVisit(VisitRecord::make(20260228, "Dr. E", "fifth", ""));
    CHECK(copy.visitCount() == 4 && c.visitCount() == 5);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 1)
        g_data = argv[1];
    else if (!std::ifstream(g_data + "/hospital_graph.txt"))
        g_data = "../data";

    int failed = 0;
    std::cout << "VitalRoute scenario tests (C++)\n";
    failed += !run("T5  Triage order and bed assignment (ICU never sent to a nearer ward)",
                   testTriageAndRouting);
    failed += !run("T6  Turn-by-turn directions with lift and stairs", testNavigation);
    failed += !run("T7  Billing with insurance matches hand calculation", testBillingWithInsurance);
    failed += !run("T8  Full journey: register to reload", testFullJourney);
    failed += !run("X6  Operation theatre slots reuse released slot", testTheatreSlots);
    failed += !run("X7  Transfer desk: urgent at front, routine at back", testTransferDesk);
    failed += !run("X8  Next available doctor by lower-bound search", testNextAvailableDoctor);
    failed += !run("X9  Registry lookup, undo of registration, visit order", testRegistryAndUndo);
    std::cout << "scenarios: " << failed << " test(s) failed\n";
    return failed;
}
