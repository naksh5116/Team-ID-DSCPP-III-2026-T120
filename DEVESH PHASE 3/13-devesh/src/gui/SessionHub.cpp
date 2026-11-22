// SessionHub.cpp - routes every change through one place and announces it.
//
// Slots connected to these signals must not throw: the signals are also
// emitted while an exception from the session is on its way to the caller.
#include "SessionHub.hpp"

#include <stdexcept>
#include <utility>

SessionHub::SessionHub(QString dataDir, QObject* parent)
    : QObject(parent), dataDir_(std::move(dataDir))
{
}

SessionHub::~SessionHub() = default;

HospitalSession& SessionHub::live()
{
    if (!session_)
        throw std::logic_error("the session has not been started");
    return *session_;
}

HospitalSession::StartupReport SessionHub::start()
{
    auto fresh = std::make_unique<HospitalSession>(dataDir_.toStdString());
    const HospitalSession::StartupReport report = fresh->startUp();
    session_ = std::move(fresh);
    return report;
}

HospitalSession::StartupReport SessionHub::reload()
{
    // Build the new session completely before replacing the old one, so a
    // bad record file leaves the running session untouched.
    auto fresh = std::make_unique<HospitalSession>(dataDir_.toStdString());
    const HospitalSession::StartupReport report = fresh->startUp();
    session_ = std::move(fresh);
    emitChanges(Everything);
    Q_EMIT reloaded();
    return report;
}

void SessionHub::emitChanges(unsigned changes)
{
    if (changes & Patients)
        Q_EMIT patientsChanged();
    if (changes & Queue)
        Q_EMIT queueChanged();
    if (changes & Beds)
        Q_EMIT bedsChanged();
    if (changes & Bills)
        Q_EMIT billsChanged();
    if (changes & Theatres)
        Q_EMIT theatresChanged();
    if (changes & Transfers)
        Q_EMIT transfersChanged();
    if (changes & Staff)
        Q_EMIT staffChanged();
}

void SessionHub::refreshAll()
{
    emitChanges(Everything);
}

// ----------------------------------------------------------- reception --

const Patient& SessionHub::registerEmergency(const std::string& name, int age,
                                             const std::string& phone,
                                             const std::string& complaint, int severity,
                                             const std::optional<Insurance>& cover)
{
    Notify n(*this, Queue | Patients | Bills);
    Patient& p = live().registerEmergency(name, age, phone, complaint, severity);
    if (cover)
        live().attachInsurance(p.id(), *cover);
    return p;
}

const Patient& SessionHub::registerOpd(const std::string& name, int age,
                                       const std::string& phone, const std::string& complaint,
                                       const std::string& department,
                                       const std::optional<Insurance>& cover)
{
    Notify n(*this, Queue | Patients | Bills);
    Patient& p = live().registerOpd(name, age, phone, complaint, department);
    if (cover)
        live().attachInsurance(p.id(), *cover);
    return p;
}

void SessionHub::attachInsurance(const std::string& patientId, const Insurance& cover)
{
    Notify n(*this, Bills);
    live().attachInsurance(patientId, cover);
}

// ----------------------------------------------------------- treatment --

HospitalSession::TreatResult SessionHub::treatNext(
    const std::string& diagnosis, const std::string& prescriptions,
    const std::optional<HospitalSession::MedicineLine>& medicine)
{
    HospitalSession::TreatResult r;
    {
        Notify n(*this, Queue | Patients | Bills | Staff | Beds);
        r = live().treatNext(diagnosis, prescriptions, medicine);
    }
    // After the tables have refreshed, so a Patient window sees the bed.
    if (r.outcome == HospitalSession::TreatResult::Outcome::Admitted && r.patient)
        Q_EMIT routeAssigned(QString::fromStdString(r.patient->id()));
    return r;
}

void SessionHub::addVisitNote(const std::string& patientId, int date, const std::string& doctor,
                              const std::string& diagnosis, const std::string& prescriptions)
{
    Notify n(*this, Patients);
    live().addVisitNote(patientId, date, doctor, diagnosis, prescriptions);
}

// ---------------------------------------------- theatres and transfers --

OTScheduler::Booking SessionHub::bookTheatre(const std::string& theatre,
                                             const std::string& patientId,
                                             const std::string& procedure, double surgeonFee)
{
    Notify n(*this, Theatres | Bills);
    return live().bookTheatre(theatre, patientId, procedure, surgeonFee);
}

void SessionHub::releaseTheatreSlot(const std::string& theatre, int slot)
{
    Notify n(*this, Theatres);
    live().releaseTheatreSlot(theatre, slot);
}

TransferRequest SessionHub::requestTransfer(bool urgent, bool ambulance,
                                            const std::string& patientId,
                                            const std::string& from, const std::string& to)
{
    Notify n(*this, Transfers);
    return live().requestTransfer(urgent, ambulance, patientId, from, to);
}

HospitalSession::DispatchResult SessionHub::dispatchNextTransfer()
{
    Notify n(*this, Transfers);
    HospitalSession::DispatchResult d = live().dispatchNextTransfer();
    if (!d.chargedBillId.empty())
        n.add(Bills);
    return d;
}

bool SessionHub::cancelLastRoutineTransfer(TransferRequest& out)
{
    Notify n(*this, Transfers);
    return live().cancelLastRoutineTransfer(out);
}

// --------------------------------------------------- billing, discharge --

void SessionHub::addManualCharge(const std::string& patientId, const Charge& charge,
                                 const std::string& authoriserId)
{
    Notify n(*this, Bills);
    live().addManualCharge(patientId, charge, authoriserId);
}

const Bill& SessionHub::discharge(const std::string& patientId, int date,
                                  const std::optional<Insurance>& cover)
{
    Notify n(*this, Patients | Beds | Bills);
    return live().discharge(patientId, date, cover);
}

// ------------------------------------------------------ undo and save --

HospitalSession::UndoResult SessionHub::undoLast()
{
    Notify n(*this, Everything);
    return live().undoLast();
}

PersistenceManager::Counts SessionHub::save()
{
    const PersistenceManager::Counts c = live().save();
    emitChanges(Everything);
    Q_EMIT saved();
    return c;
}
