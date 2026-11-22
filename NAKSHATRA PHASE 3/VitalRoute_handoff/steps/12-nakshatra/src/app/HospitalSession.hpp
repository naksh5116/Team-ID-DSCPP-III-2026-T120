// HospitalSession.hpp - one running hospital: every module, wired together.
//
// This is the application logic that used to live inside the console's
// VitalRouteApp, without any input or output. Methods take plain values and
// return values (or throw the existing domain exceptions); none of them
// reads stdin or prints. The console (src/main.cpp) and the Qt GUI
// (src/gui/) are both thin front ends over one HospitalSession.
//
// The undo journal - the last registration or manual charge - is the C
// engine's vr_stack.
//
// A few console actions ask questions half-way through (an outpatient's
// diagnosis after the doctor is assigned; insurance after the bed is
// released at discharge). Those actions are split into two calls here, so
// the console can keep its exact prompt order; the GUI simply makes both
// calls one after the other.
#ifndef VR_HOSPITAL_SESSION_HPP
#define VR_HOSPITAL_SESSION_HPP

#include "BillingEngine.hpp"
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

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class HospitalSession {
public:
    static constexpr double kIcuBedPerDay = 6000.0;
    static constexpr double kWardBedPerDay = 2500.0;
    static const char* const kEmergencySpecialty;     // "Emergency Medicine"

    // ------------------------------------------------------ results --

    struct StartupReport {
        bool loadedFromRecords = false;        // false = first run, sample data seeded
        PersistenceManager::Counts loaded;     // valid when loadedFromRecords
        std::vector<std::string> warnings;     // saved beds that could not be re-occupied
    };

    // Outcome of treating the next emergency or outpatient.
    struct TreatResult {
        enum class Outcome {
            NobodyWaiting,    // both lines empty
            NoBed,            // no bed of the required type; patient re-queued at the head
            ReleasedFromEr,   // ESI 4-5: treated in the ER, no bed needed
            Admitted,         // bed assigned, route computed
            Outpatient        // OPD consultation recorded
        };
        Outcome outcome = Outcome::NobodyWaiting;
        Patient* patient = nullptr;
        const Doctor* doctor = nullptr;        // nullptr when nobody has the specialty
        int doctorStart = 0;                   // minutes since midnight
        std::string doctorName;                // doctor's name, or "duty doctor"
        std::string error;                     // NoBed: why no bed could be allocated
        BedAssignment bed{};                   // Admitted only
        Route route;                           // Admitted only
        std::string routeError;                // Admitted, but the route could not be drawn
        double billSoFar = 0.0;                // Outpatient: projected bill total
    };

    // First half of an outpatient consultation: taken off the OPD line and
    // a doctor booked. Finish it with completeOpd().
    struct OpdVisit {
        Patient* patient = nullptr;
        const Doctor* doctor = nullptr;
        int doctorStart = 0;
        std::string doctorName;
    };

    struct MedicineLine {
        std::string name;
        int quantity = 1;
        double unitPrice = 0.0;
    };

    struct DispatchResult {
        bool dispatched = false;               // false when no request was pending
        TransferRequest request{};
        std::string chargedBillId;             // set when an ambulance charge was added
    };

    enum class DischargeBlock { None, StillWaiting, AlreadyDischarged };

    // First half of a discharge: further bed days charged and the bed freed.
    struct BedRelease {
        int date = 0;                          // discharge date actually used
        bool dateClamped = false;              // requested date was before admission
        std::string releasedBedLabel;          // empty when the patient had no bed
    };

    struct UndoResult {
        enum class Kind {
            Nothing,               // journal empty
            RegistrationUndone,
            ChargeUndone,
            RegistrationDropped,   // patient already seen; entry discarded
            ChargeDropped          // bill changed or settled since; entry discarded
        };
        Kind kind = Kind::Nothing;
        std::string patientId;
        std::string note;                      // "registration of ...", "charge '...' on ..."
        double amount = 0.0;                   // ChargeUndone: cost of the removed charge
    };

    // ------------------------------------------------- life cycle --

    explicit HospitalSession(std::string dataDir);
    ~HospitalSession();
    HospitalSession(const HospitalSession&) = delete;
    HospitalSession& operator=(const HospitalSession&) = delete;

    // Loads the floor plan, beds and theatres, then either the saved records
    // (rebuilding queues and bed occupancy) or the sample roster and patients.
    // Throws std::runtime_error on a malformed data file.
    StartupReport startUp();

    // Writes data/records/. Throws std::runtime_error if a file cannot be written.
    PersistenceManager::Counts save();

    // ------------------------------------------------ reception --

    // Throws InvalidSeverityException unless 1 <= severity <= 5. Opens a
    // bill, joins the emergency heap and records an undo entry.
    Patient& registerEmergency(const std::string& name, int age, const std::string& phone,
                               const std::string& complaint, int severity);
    // Opens a bill, joins the OPD line and records an undo entry.
    Patient& registerOpd(const std::string& name, int age, const std::string& phone,
                         const std::string& complaint, const std::string& department);
    // Specialties an outpatient can be booked into (all but emergency).
    std::vector<std::string> opdDepartments() const;
    // Waiting emergencies, most urgent first.
    std::vector<const Patient*> emergencyOrder() const;
    // 1-based place in the emergency queue (0 when not waiting there).
    std::size_t emergencyPosition(const std::string& patientId) const;

    void attachInsurance(const std::string& patientId, const Insurance& cover);

    // ------------------------------------------------- treatment --

    // Emergencies first (heap order), then outpatients (FIFO). An
    // outpatient is finished with the given notes in one step.
    TreatResult treatNext(const std::string& diagnosis = "", const std::string& prescriptions = "",
                          const std::optional<MedicineLine>& medicine = std::nullopt);
    TreatResult treatNextEmergency();          // requires a waiting emergency
    OpdVisit beginOpd();                       // requires a waiting outpatient
    // Empty diagnosis/prescriptions become "Routine review"/"None".
    // Returns the outpatient's projected bill total.
    double completeOpd(const OpdVisit& visit, const std::string& diagnosis,
                       const std::string& prescriptions,
                       const std::optional<MedicineLine>& medicine);

    // ---------------------------------------------- patient record --

    Patient& findPatient(const std::string& id);              // throws PatientNotFoundException
    const Patient& findPatient(const std::string& id) const;
    const Doctor* findDoctor(const std::string& id) const;    // nullptr when absent
    const Staff* findStaff(const std::string& id) const;      // nullptr when absent
    // Empty prescriptions become "None".
    void addVisitNote(const std::string& patientId, int date, const std::string& doctor,
                      const std::string& diagnosis, const std::string& prescriptions);
    // Walking route from the ER to the patient's bed. Throws
    // std::runtime_error when the patient has no bed, NoRouteFoundException
    // when it cannot be reached.
    Route routeForPatient(const std::string& patientId) const;

    // Registered on dates from..to inclusive (swapped if reversed), date order.
    std::vector<Patient*> admittedBetween(int fromYmd, int toYmd) const;

    // ----------------------------------------------------- staff --

    std::vector<std::string> specialties() const;
    const Doctor* nextAvailableDoctor(const std::string& specialty) const;

    // ------------------------------------------ theatres, transfers --

    // Books the next free slot and charges the surgery to the patient.
    // Throws PatientNotFoundException or SlotUnavailableException.
    OTScheduler::Booking bookTheatre(const std::string& theatre, const std::string& patientId,
                                     const std::string& procedure, double surgeonFee);
    void releaseTheatreSlot(const std::string& theatre, int slot);   // 0-based

    // Urgent requests go to the front, routine ones to the back. A non-empty
    // patient ID must exist (PatientNotFoundException).
    TransferRequest requestTransfer(bool urgent, bool ambulance, const std::string& patientId,
                                    const std::string& from, const std::string& to);
    // Dispatches from the front; an ambulance run for a patient with an open
    // bill is charged to it.
    DispatchResult dispatchNextTransfer();
    bool cancelLastRoutineTransfer(TransferRequest& out);

    // --------------------------------------------------- billing --

    Bill& billFor(const std::string& patientId);              // throws PatientNotFoundException
    const Bill* findBill(const std::string& patientId) const;
    std::vector<const Bill*> listBills(bool byAmount) const;  // merge-sorted
    // A charge entered by hand. Only an administrator may authorise it
    // (std::runtime_error otherwise, or if the staff ID is unknown).
    // Records an undo entry.
    void addManualCharge(const std::string& patientId, const Charge& charge,
                         const std::string& authoriserId);

    // ------------------------------------------------- discharge --

    // Why the patient cannot be discharged (None when they can).
    // Throws PatientNotFoundException.
    DischargeBlock dischargeBlocker(const std::string& patientId) const;
    // Charges further bed days, frees the bed. Throws std::logic_error
    // when dischargeBlocker() is not None.
    BedRelease prepareDischarge(const std::string& patientId, int date);
    // Applies insurance once, settles the bill, marks the patient discharged.
    const Bill& settleDischarge(const std::string& patientId, int date);
    // Both halves in one call; `cover` is attached first when given.
    const Bill& discharge(const std::string& patientId, int date,
                          const std::optional<Insurance>& cover = std::nullopt);

    // ------------------------------------------------------ undo --

    UndoResult undoLast();
    bool canUndo() const;
    std::string nextUndoNote() const;          // "" when the journal is empty

    // ------------------------------------------- read-only access --

    const std::string& dataDir() const { return dataDir_; }
    const std::string& recordsDir() const { return store_.directory(); }
    const HospitalMap& map() const { return map_; }
    const Navigator& navigator() const { return navigator_; }
    const ResourceRouter& router() const { return router_; }
    const PatientRegistry& patients() const { return patients_; }
    const StaffRegistry& staff() const { return staff_; }
    const TriageUnit& triage() const { return triage_; }
    const OTScheduler& theatres() const { return theatres_; }
    const TransferDesk& transfers() const { return transfers_; }
    const BillingEngine& billing() const { return billing_; }

private:
    enum UndoKind { UNDO_REGISTRATION = 1, UNDO_CHARGE = 2 };

    struct UndoEntry {            // plain struct: stored by value in vr_stack
        int kind;
        char patientId[24];
        unsigned long chargeCount;  // bill size right after a charge entry
        char note[96];
    };

    void seed();
    std::vector<std::string> restoreRuntimeState();
    Patient& admit(const std::string& name, int age, const std::string& phone, PatientKind kind,
                   int severity, const std::string& complaint, const std::string& department);
    void pushUndo(UndoKind kind, const std::string& patientId, const std::string& note);

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

#endif // VR_HOSPITAL_SESSION_HPP
