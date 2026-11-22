// check_hub.cpp - SessionHub: which signals each action emits.
#include "GuiCheck.hpp"

namespace {

// Counts every SessionHub signal between two calls to reset().
class SignalRecorder {
public:
    explicit SignalRecorder(SessionHub& hub)
    {
        auto count = [this](const char* name) { return [this, name] { ++counts_[QLatin1String(name)]; }; };
        QObject::connect(&hub, &SessionHub::queueChanged, count("queue"));
        QObject::connect(&hub, &SessionHub::patientsChanged, count("patients"));
        QObject::connect(&hub, &SessionHub::bedsChanged, count("beds"));
        QObject::connect(&hub, &SessionHub::billsChanged, count("bills"));
        QObject::connect(&hub, &SessionHub::theatresChanged, count("theatres"));
        QObject::connect(&hub, &SessionHub::transfersChanged, count("transfers"));
        QObject::connect(&hub, &SessionHub::staffChanged, count("staff"));
        QObject::connect(&hub, &SessionHub::saved, count("saved"));
        QObject::connect(&hub, &SessionHub::reloaded, count("reloaded"));
        QObject::connect(&hub, &SessionHub::routeAssigned,
                         [this](const QString& id) { routes_ << id; });
    }
    void reset()
    {
        counts_.clear();
        routes_.clear();
    }
    // Signals fired since reset(), sorted, e.g. "bills patients queue".
    QString fired() const { return QStringList(counts_.keys()).join(QLatin1Char(' ')); }
    const QStringList& routes() const { return routes_; }

private:
    QMap<QString, int> counts_;
    QStringList routes_;
};

} // namespace

void hubChecks(const QString& dataDir)
{
    section("SessionHub signals");
    SessionHub hub(dataDir);
    SignalRecorder rec(hub);

    const HospitalSession::StartupReport start = hub.start();
    const HospitalSession& s = hub.session();
    report(!start.loadedFromRecords && s.triage().emergencyCount() == 6 &&
               s.triage().opdCount() == 2,
           "H1  First start seeds 6 emergencies and 2 outpatients");
    const int icuBefore = s.router().freeBeds(CareUnit::ICU);

    // Registration: queue, patients, bills - and nothing about beds.
    rec.reset();
    const Patient& p = hub.registerEmergency("Check Patient", 50, "", "Collapse", 1,
                                             Insurance("CGHS", "CG-1", Insurance::Kind::Flat, 500.0));
    const std::string newId = p.id();
    report(rec.fired() == QStringLiteral("bills patients queue") &&
               s.findBill(newId) && s.findBill(newId)->insurance(),
           "H2  Register emergency emits queue, patients, bills", str(rec.fired()));

    // Invalid severity: exception passes through, nothing registered.
    const std::size_t before = s.patients().size();
    bool threw = false;
    try {
        hub.registerEmergency("Bad", 30, "", "Pain", 9, std::nullopt);
    } catch (const InvalidSeverityException&) {
        threw = true;
    }
    report(threw && s.patients().size() == before,
           "H3  Invalid ESI throws InvalidSeverityException, nothing added");

    // Treat: the first ESI 1 in arrival order gets an ICU bed and a route.
    rec.reset();
    const HospitalSession::TreatResult t = hub.treatNext();
    report(t.outcome == HospitalSession::TreatResult::Outcome::Admitted && t.patient &&
               t.patient->id() == "VR-2026-000004" && !t.route.steps.empty() &&
               s.router().freeBeds(CareUnit::ICU) == icuBefore - 1,
           "H4  Treat next admits VR-2026-000004 to ICU with a route",
           t.patient ? t.patient->id() + ", " + t.bed.label : "no patient");
    report(rec.fired() == QStringLiteral("beds bills patients queue staff") &&
               rec.routes() == QStringList{QStringLiteral("VR-2026-000004")},
           "H5  Treat emits queue, patients, beds, bills, staff and routeAssigned(id)",
           str(rec.fired() + QStringLiteral(" / ") + rec.routes().join(QLatin1Char(','))));

    // Route recomputed later for the Patient window matches the one returned.
    const Route again = s.routeForPatient("VR-2026-000004");
    report(again.path == t.route.path && again.steps == t.route.steps,
           "H6  routeForPatient matches the route given at treatment");

    // Theatres.
    rec.reset();
    const OTScheduler::Booking b = hub.bookTheatre("OT-1", "VR-2026-000004", "Angiography", 25000.0);
    report(b.slot == 0 && rec.fired() == QStringLiteral("bills theatres"),
           "H7  Book theatre emits theatres, bills", str(rec.fired()));
    rec.reset();
    hub.releaseTheatreSlot("OT-1", 0);
    threw = false;
    try {
        hub.releaseTheatreSlot("OT-1", 0);
    } catch (const SlotUnavailableException&) {
        threw = true;
    }
    report(threw && rec.fired() == QStringLiteral("theatres"),
           "H8  Release slot emits theatres; releasing twice throws", str(rec.fired()));

    // Transfer desk: urgent to the front, routine to the back.
    rec.reset();
    hub.requestTransfer(false, false, "", "General Ward A", "Radiology");
    hub.requestTransfer(true, true, "VR-2026-000005", "Roadside", "Central Emergency Room");
    const auto pending = s.transfers().pending();
    report(pending.size() == 2 && pending.front().urgent == 1 &&
               rec.fired() == QStringLiteral("transfers"),
           "H9  Urgent request goes to the front; emits transfers only", str(rec.fired()));
    rec.reset();
    const HospitalSession::DispatchResult d = hub.dispatchNextTransfer();
    report(d.dispatched && !d.chargedBillId.empty() &&
               rec.fired() == QStringLiteral("bills transfers"),
           "H10 Dispatching a patient ambulance also emits bills", str(rec.fired()));

    // Manual charge then undo.
    rec.reset();
    const std::size_t charges = s.findBill(newId)->chargeCount();
    hub.addManualCharge(newId, TestCharge("Chest X-ray", vrclock::today(), 900.0, false), "AD-01");
    const bool added = s.findBill(newId)->chargeCount() == charges + 1 &&
                       rec.fired() == QStringLiteral("bills");
    threw = false;
    try {
        hub.addManualCharge(newId, TestCharge("X", vrclock::today(), 1.0, false), "DR-01");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    report(added && threw && s.findBill(newId)->chargeCount() == charges + 1,
           "H11 Manual charge by an admin emits bills; a doctor is refused");
    rec.reset();
    const HospitalSession::UndoResult u = hub.undoLast();
    report(u.kind == HospitalSession::UndoResult::Kind::ChargeUndone &&
               s.findBill(newId)->chargeCount() == charges &&
               rec.fired() == QStringLiteral("beds bills patients queue staff theatres transfers"),
           "H12 Undo removes the charge and refreshes every signal", str(rec.fired()));

    // Discharge frees the bed.
    rec.reset();
    const Bill& settled = hub.discharge("VR-2026-000004", vrclock::today(), std::nullopt);
    report(settled.settled() && s.router().freeBeds(CareUnit::ICU) == icuBefore &&
               rec.fired() == QStringLiteral("beds bills patients"),
           "H13 Discharge settles the bill, frees the ICU bed", str(rec.fired()));

    // Save, reload: the same records come back (as test T8 does for the console).
    const std::size_t patientsSaved = s.patients().size();
    const std::size_t waitingE = s.triage().emergencyCount();
    const std::size_t waitingO = s.triage().opdCount();
    const std::string peek = s.triage().peekEmergency();
    rec.reset();
    hub.save();
    const bool savedAll = rec.fired().contains(QStringLiteral("saved")) &&
                          rec.fired().contains(QStringLiteral("queue"));
    rec.reset();
    const HospitalSession::StartupReport re = hub.reload();
    const HospitalSession& s2 = hub.session();
    report(savedAll && re.loadedFromRecords && rec.fired().contains(QStringLiteral("reloaded")) &&
               s2.patients().size() == patientsSaved && s2.triage().emergencyCount() == waitingE &&
               s2.triage().opdCount() == waitingO && s2.triage().peekEmergency() == peek &&
               s2.findPatient("VR-2026-000004").status() == PatientStatus::Discharged,
           "H14 Save then reload restores patients, queues and statuses",
           str(rec.fired()));
}

