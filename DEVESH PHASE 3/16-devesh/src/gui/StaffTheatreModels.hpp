// StaffTheatreModels.hpp - doctor availability and theatre slots as tables.
//
//   DoctorAvailabilityModel  StaffRegistry's availability array, in array
//                            order (sorted by specialty, then next-free time)
//   TheatreModel             OTScheduler's slot table, every theatre
#ifndef VR_GUI_STAFF_THEATRE_MODELS_HPP
#define VR_GUI_STAFF_THEATRE_MODELS_HPP

#include "SnapshotTableModel.hpp"

#include <QList>
#include <QString>

class OTScheduler;

class DoctorAvailabilityModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit DoctorAvailabilityModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Specialty, DoctorName, StaffId, NextFree, Seen };

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

// One theatre's day, read through OTScheduler's public interface.
struct TheatreDay {
    struct Slot {
        QString time;        // 08:00-09:00
        bool booked = false;
        QString patientId;
        QString procedure;
    };
    QString name;
    QList<Slot> slots;
    int freeCount = 0;
    QList<int> ringOrder;    // free slot numbers (1-based) in circular-queue order
};

// The schedule as Qt data. The reading is shared with the web version:
// src/app/TheatreSchedule takes the procedure and the free-slot ring order
// from OTScheduler::printSchedule() (src/modules may not change) - read-only
// use of the public interface.
QList<TheatreDay> readTheatreDays(const OTScheduler& ot);

class TheatreModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit TheatreModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Theatre, Slot, Time, Status, PatientId, PatientName, Procedure };

    // Key format "OT-1|0" (theatre | 0-based slot).
    static QString theatreOf(const QString& key);
    static int slotOf(const QString& key);

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

#endif // VR_GUI_STAFF_THEATRE_MODELS_HPP
