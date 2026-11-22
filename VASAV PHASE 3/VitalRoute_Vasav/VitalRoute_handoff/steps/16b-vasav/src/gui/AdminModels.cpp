// AdminModels.cpp
#include "AdminModels.hpp"

#include "SessionHub.hpp"
#include "Text.hpp"

using Text::qs;

// --------------------------------------------------------------- bills --

BillsListModel::BillsListModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Bill"), QStringLiteral("Patient ID"), QStringLiteral("Opened"),
                          QStringLiteral("Total"), QStringLiteral("State")},
                         parent)
{
    refreshOn(&SessionHub::billsChanged);
    refresh();
}

void BillsListModel::setSortByAmount(bool byAmount)
{
    byAmount_ = byAmount;
    refresh();
}

void BillsListModel::build(QList<Row>& rows, QStringList& keys) const
{
    for (const Bill* b : hub().session().listBills(byAmount_)) {   // merge-sorted
        rows.append({mono(qs(b->billId())), mono(qs(b->patientId())), text(Text::date(b->openedOn())),
                     amount(Text::money(b->total())),
                     text(b->settled() ? QStringLiteral("Settled") : QStringLiteral("Open"))});
        keys.append(qs(b->patientId()));
    }
}

// ---------------------------------------------------------- admissions --

AdmissionsModel::AdmissionsModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Registered"), QStringLiteral("Patient ID"), QStringLiteral("Name"),
                          QStringLiteral("Type"), QStringLiteral("Status")},
                         parent)
{
    refreshOn(&SessionHub::patientsChanged);
}

void AdmissionsModel::setRange(int fromYmd, int toYmd)
{
    from_ = fromYmd;
    to_ = toYmd;
    refresh();
}

void AdmissionsModel::build(QList<Row>& rows, QStringList& keys) const
{
    if (!from_ || !to_)
        return;
    for (const Patient* p : hub().session().admittedBetween(from_, to_)) {   // AVL range walk
        rows.append({text(Text::date(p->admissionDate())), mono(qs(p->id())), text(qs(p->name())),
                     text(p->isEmergency() ? QStringLiteral("Emergency") : QStringLiteral("Outpatient")),
                     text(Text::statusText(p->status()))});
        keys.append(qs(p->id()));
    }
}

// --------------------------------------------------------------- staff --

StaffRosterModel::StaffRosterModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Staff ID"), QStringLiteral("Name"), QStringLiteral("Role"),
                          QStringLiteral("Department"), QStringLiteral("Shift"), QStringLiteral("Details")},
                         parent)
{
    refreshOn(&SessionHub::staffChanged);
    refresh();
}

void StaffRosterModel::build(QList<Row>& rows, QStringList& keys) const
{
    const StaffRegistry& staff = hub().session().staff();
    auto add = [&](const Staff* s, const QString& detail) {
        rows.append({mono(qs(s->id())), text(qs(s->name())), text(qs(s->role())),
                     text(qs(s->department())), text(qs(s->shift())), text(detail)});
        keys.append(qs(s->id()));
    };
    for (const Doctor* d : staff.doctors())
        add(d, QStringLiteral("%1, consultation %2").arg(qs(d->specialty()), Text::money(d->consultationFee())));
    for (const Nurse* n : staff.nurses())
        add(n, QStringLiteral("Ward %1").arg(qs(n->ward())));
    for (const Admin* a : staff.admins())
        add(a, QStringLiteral("%1; may authorise manual charges").arg(qs(a->office())));
}
