// StaffTheatreModels.cpp
#include "StaffTheatreModels.hpp"

#include "SessionHub.hpp"
#include "TheatreSchedule.hpp"
#include "Text.hpp"

using Text::qs;

// ------------------------------------------------------ availability --

DoctorAvailabilityModel::DoctorAvailabilityModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Specialty"), QStringLiteral("Doctor"), QStringLiteral("Staff ID"),
                          QStringLiteral("Next free"), QStringLiteral("Patients seen")},
                         parent)
{
    refreshOn(&SessionHub::staffChanged);
    refresh();
}

void DoctorAvailabilityModel::build(QList<Row>& rows, QStringList& keys) const
{
    const int now = vrclock::minutesNow();
    for (const Doctor* d : hub().session().staff().availability()) {   // array order
        rows.append({text(qs(d->specialty())), text(qs(d->name())), mono(qs(d->id())),
                     text(d->nextFree() <= now ? QStringLiteral("Free now")
                                               : QStringLiteral("Busy until %1").arg(Text::clock(d->nextFree()))),
                     text(QString::number(d->patientsSeen()))});
        keys.append(qs(d->id()));
    }
}

// ----------------------------------------------------------- theatres --

QList<TheatreDay> readTheatreDays(const OTScheduler& ot)
{
    // The reading itself is shared with the web version (src/app/TheatreSchedule).
    QList<TheatreDay> days;
    for (const TheatreDayInfo& info : readTheatreSchedule(ot)) {
        TheatreDay day;
        day.name = qs(info.name);
        day.freeCount = info.freeCount;
        for (int n : info.ringOrder)
            day.ringOrder.append(n);
        for (const TheatreSlotInfo& s : info.slots) {
            TheatreDay::Slot slot;
            slot.time = qs(s.time);
            slot.booked = s.booked;
            slot.patientId = qs(s.patientId);
            slot.procedure = qs(s.procedure);
            day.slots.append(slot);
        }
        days.append(day);
    }
    return days;
}

TheatreModel::TheatreModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Theatre"), QStringLiteral("Slot"), QStringLiteral("Time"),
                          QStringLiteral("Status"), QStringLiteral("Patient ID"), QStringLiteral("Patient"),
                          QStringLiteral("Procedure")},
                         parent)
{
    refreshOn(&SessionHub::theatresChanged);
    refreshOn(&SessionHub::patientsChanged);
    refresh();
}

QString TheatreModel::theatreOf(const QString& key)
{
    return key.section(QLatin1Char('|'), 0, 0);
}

int TheatreModel::slotOf(const QString& key)
{
    return key.section(QLatin1Char('|'), 1, 1).toInt();
}

void TheatreModel::build(QList<Row>& rows, QStringList& keys) const
{
    const HospitalSession& s = hub().session();
    for (const TheatreDay& day : readTheatreDays(s.theatres())) {
        for (int i = 0; i < day.slots.size(); ++i) {
            const TheatreDay::Slot& slot = day.slots.at(i);
            QString patient;
            if (slot.booked)
                if (const Patient* p = s.patients().tryFind(Text::ss(slot.patientId)))
                    patient = qs(p->name());
            rows.append({text(day.name), text(QString::number(i + 1)), mono(slot.time),
                         text(slot.booked ? QStringLiteral("Booked") : QStringLiteral("Free")),
                         mono(slot.patientId), text(patient), text(slot.procedure)});
            keys.append(day.name + QLatin1Char('|') + QString::number(i));
        }
    }
}
