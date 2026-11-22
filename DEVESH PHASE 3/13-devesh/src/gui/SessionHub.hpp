// SessionHub.hpp - the one HospitalSession every window shares.
//
// The GUI runs as one process with several top-level windows (one per role,
// often one per monitor). All of them read the same HospitalSession, and
// every change goes through this hub. After each change the hub emits only
// the signals for what changed; each window connects to the signals it
// needs and refreshes its tables. Nothing polls and there are no timers.
//
//   change                     signals emitted
//   -------------------------  --------------------------------------------
//   register a patient         queueChanged, patientsChanged, billsChanged
//   treat next patient         queueChanged, patientsChanged, billsChanged,
//                              staffChanged, bedsChanged; routeAssigned(id)
//                              when a bed was assigned
//   add a visit note           patientsChanged
//   book / release theatre     theatresChanged (+ billsChanged on booking)
//   transfer desk              transfersChanged (+ billsChanged when an
//                              ambulance run is charged)
//   insurance, manual charge   billsChanged
//   discharge                  patientsChanged, bedsChanged, billsChanged
//   undo, save, reload         every signal (and saved / reloaded)
//
// Exceptions from the session (the existing domain exceptions) pass through
// unchanged so the window can show the message. The change signals are
// still emitted, so no window is left showing a half-applied state.
#ifndef VR_GUI_SESSION_HUB_HPP
#define VR_GUI_SESSION_HUB_HPP

#include "HospitalSession.hpp"

#include <QObject>
#include <QString>

#include <memory>
#include <optional>
#include <string>

class SessionHub : public QObject {
    Q_OBJECT
public:
    explicit SessionHub(QString dataDir, QObject* parent = nullptr);
    ~SessionHub() override;

    // Creates the session and loads saved records or the sample data.
    // Throws std::runtime_error on a malformed data file.
    HospitalSession::StartupReport start();
    // Throws away everything in memory and starts again from data/ (as a
    // console restart would). Theatre bookings and transfer requests are not
    // part of the saved records, so they start empty. The current session is
    // kept if loading fails.
    HospitalSession::StartupReport reload();

    bool isStarted() const { return session_ != nullptr; }
    const HospitalSession& session() const { return *session_; }
    const QString& dataDir() const { return dataDir_; }

    // ------------------------------------------------- mutations --

    const Patient& registerEmergency(const std::string& name, int age, const std::string& phone,
                                     const std::string& complaint, int severity,
                                     const std::optional<Insurance>& cover);
    const Patient& registerOpd(const std::string& name, int age, const std::string& phone,
                               const std::string& complaint, const std::string& department,
                               const std::optional<Insurance>& cover);
    void attachInsurance(const std::string& patientId, const Insurance& cover);

    HospitalSession::TreatResult treatNext(
        const std::string& diagnosis = "", const std::string& prescriptions = "",
        const std::optional<HospitalSession::MedicineLine>& medicine = std::nullopt);
    void addVisitNote(const std::string& patientId, int date, const std::string& doctor,
                      const std::string& diagnosis, const std::string& prescriptions);

    OTScheduler::Booking bookTheatre(const std::string& theatre, const std::string& patientId,
                                     const std::string& procedure, double surgeonFee);
    void releaseTheatreSlot(const std::string& theatre, int slot);

    TransferRequest requestTransfer(bool urgent, bool ambulance, const std::string& patientId,
                                    const std::string& from, const std::string& to);
    HospitalSession::DispatchResult dispatchNextTransfer();
    bool cancelLastRoutineTransfer(TransferRequest& out);

    void addManualCharge(const std::string& patientId, const Charge& charge,
                         const std::string& authoriserId);
    const Bill& discharge(const std::string& patientId, int date,
                          const std::optional<Insurance>& cover);

    HospitalSession::UndoResult undoLast();
    PersistenceManager::Counts save();

    // Re-emits every change signal (F5 in any window).
    void refreshAll();

Q_SIGNALS:
    void queueChanged();
    void patientsChanged();
    void bedsChanged();
    void billsChanged();
    void theatresChanged();
    void transfersChanged();
    void staffChanged();
    void routeAssigned(const QString& patientId);
    void saved();
    void reloaded();

private:
    enum Change : unsigned {
        Queue = 1u << 0,
        Patients = 1u << 1,
        Beds = 1u << 2,
        Bills = 1u << 3,
        Theatres = 1u << 4,
        Transfers = 1u << 5,
        Staff = 1u << 6,
        Everything = 0x7Fu
    };

    // Emits the change signals in `changes` when it goes out of scope, so
    // they are sent whether the action returned or threw.
    class Notify {
    public:
        Notify(SessionHub& hub, unsigned changes) : hub_(hub), changes_(changes) {}
        ~Notify() { hub_.emitChanges(changes_); }
        void add(unsigned changes) { changes_ |= changes; }
        Notify(const Notify&) = delete;
        Notify& operator=(const Notify&) = delete;
    private:
        SessionHub& hub_;
        unsigned changes_;
    };

    void emitChanges(unsigned changes);
    HospitalSession& live();

    QString dataDir_;
    std::unique_ptr<HospitalSession> session_;
};

#endif // VR_GUI_SESSION_HUB_HPP
