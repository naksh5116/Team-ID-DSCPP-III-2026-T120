// QueueModels.cpp - rows read straight from the triage unit and transfer desk.
#include "QueueModels.hpp"

#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"

using Text::qs;

// ------------------------------------------------------------ emergency --

EmergencyQueueModel::EmergencyQueueModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("#"), QStringLiteral("Patient ID"), QStringLiteral("Name"),
                          QStringLiteral("Age"), QStringLiteral("ESI"), QStringLiteral("Complaint"),
                          QStringLiteral("Registered")},
                         parent)
{
    refreshOn(&SessionHub::queueChanged);
    refreshOn(&SessionHub::patientsChanged);
    refresh();
}

void EmergencyQueueModel::build(QList<Row>& rows, QStringList& keys) const
{
    int position = 1;
    for (const Patient* p : hub().session().emergencyOrder()) {
        rows.append({text(QString::number(position++)), mono(qs(p->id())), text(qs(p->name())),
                     text(QString::number(p->age())),
                     esi(p->severity(), Theme::esiName(p->severity())), text(qs(p->complaint())),
                     text(Text::date(p->admissionDate()))});
        keys.append(qs(p->id()));
    }
}

// ------------------------------------------------------------------ OPD --

OpdQueueModel::OpdQueueModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("#"), QStringLiteral("Patient ID"), QStringLiteral("Name"),
                          QStringLiteral("Age"), QStringLiteral("Department"),
                          QStringLiteral("Reason for visit")},
                         parent)
{
    refreshOn(&SessionHub::queueChanged);
    refreshOn(&SessionHub::patientsChanged);
    refresh();
}

void OpdQueueModel::build(QList<Row>& rows, QStringList& keys) const
{
    const HospitalSession& s = hub().session();
    int position = 1;
    for (const std::string& id : s.triage().opdLine()) {
        const Patient* p = s.patients().tryFind(id);
        if (!p)
            continue;
        rows.append({text(QString::number(position++)), mono(qs(p->id())), text(qs(p->name())),
                     text(QString::number(p->age())), text(qs(p->department())),
                     text(qs(p->complaint()))});
        keys.append(qs(p->id()));
    }
}

// ------------------------------------------------------------ transfers --

TransferModel::TransferModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("#"), QStringLiteral("Code"), QStringLiteral("Priority"),
                          QStringLiteral("Type"), QStringLiteral("Patient ID"), QStringLiteral("From"),
                          QStringLiteral("To")},
                         parent)
{
    refreshOn(&SessionHub::transfersChanged);
    refresh();
}

void TransferModel::build(QList<Row>& rows, QStringList& keys) const
{
    int position = 1;
    for (const TransferRequest& r : hub().session().transfers().pending()) {
        rows.append({text(QString::number(position++)), mono(qs(r.code)),
                     text(r.urgent ? QStringLiteral("Urgent") : QStringLiteral("Routine")),
                     text(r.ambulance ? QStringLiteral("Ambulance") : QStringLiteral("Internal transfer")),
                     r.patientId[0] ? mono(qs(r.patientId)) : text(QStringLiteral("Not registered")),
                     text(qs(r.from)), text(qs(r.to))});
        keys.append(qs(r.code));
    }
}
