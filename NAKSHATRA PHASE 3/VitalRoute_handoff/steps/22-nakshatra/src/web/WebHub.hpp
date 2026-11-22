// WebHub.hpp - the one HospitalSession every browser shares.
//
// The web counterpart of the Qt GUI's SessionHub: it owns the session, and
// after every change the API announces exactly what changed. Each
// announcement goes to every open browser as a Server-Sent Event with the
// same names the Qt windows use:
//
//   queueChanged  patientsChanged  bedsChanged  billsChanged
//   theatresChanged  transfersChanged  staffChanged
//   routeAssigned (data: the patient ID)   saved   reloaded
//
// Every event carries a version number that grows by one per change, so a
// page that was disconnected for a moment can tell it missed something and
// refresh everything.
#ifndef VR_WEB_HUB_HPP
#define VR_WEB_HUB_HPP

#include "HospitalSession.hpp"

#include <functional>
#include <memory>
#include <string>

class WebHub {
public:
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
    using Notifier = std::function<void(const std::string& event, const std::string& data)>;

    explicit WebHub(std::string dataDir);

    // Loads saved records or the sample data. Throws std::runtime_error.
    HospitalSession::StartupReport start();
    // A fresh session from data/records (as a console restart); the current
    // one is kept if loading fails. Announces everything, then "reloaded".
    HospitalSession::StartupReport reload();

    bool isStarted() const { return session_ != nullptr; }
    const HospitalSession& session() const { return *session_; }
    HospitalSession& live();
    const std::string& dataDir() const { return dataDir_; }

    void setNotifier(Notifier notifier) { notifier_ = std::move(notifier); }
    // Sends one event per change in `changes` (and routeAssigned when
    // `routePatient` is not empty). Returns the new version.
    long announce(unsigned changes, const std::string& routePatient = "");
    // Sends a named event ("saved") with the current version.
    void announceEvent(const std::string& event);
    long version() const { return version_; }

private:
    std::string dataDir_;
    std::unique_ptr<HospitalSession> session_;
    Notifier notifier_;
    long version_ = 0;
};

#endif // VR_WEB_HUB_HPP
