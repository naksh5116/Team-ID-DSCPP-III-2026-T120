// PatientRecordView.hpp - one patient's record, read-only.
//
// Header (status, bed, doctor, bill) plus the visit history, newest first:
// Patient::visitsNewestFirst() walks the doubly linked list backwards.
// Follows patientsChanged, bedsChanged, billsChanged and reloads; if the
// patient disappears (undo of a registration) it says so.
#ifndef VR_GUI_PATIENT_RECORD_VIEW_HPP
#define VR_GUI_PATIENT_RECORD_VIEW_HPP

#include "SnapshotTableModel.hpp"

#include <QWidget>

class EsiBadge;
class QFormLayout;
class QLabel;
class QTableView;

class VisitHistoryModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit VisitHistoryModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Date, DoctorName, Diagnosis, Prescriptions };
    void setPatientId(const QString& id);

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;

private:
    QString id_;
};

class PatientRecordView : public QWidget {
    Q_OBJECT
public:
    explicit PatientRecordView(SessionHub& hub, QWidget* parent = nullptr);

    // Shows the patient, or clears the view for an empty ID. Returns false
    // (and clears) when no such patient exists.
    bool setPatientId(const QString& id);
    QString patientId() const { return id_; }
    bool hasPatient() const;

    QString fieldText(const char* objectName) const;   // for checks
    VisitHistoryModel* history() const { return history_; }

Q_SIGNALS:
    // The shown patient no longer exists (registration undone, reload).
    void patientGone(const QString& id);

private:
    void refresh();
    QLabel* addField(QFormLayout* form, const QString& label, const char* name, bool mono = false);

    SessionHub& hub_;
    QString id_;
    bool goneReported_ = false;
    QLabel* title_ = nullptr;
    EsiBadge* badge_ = nullptr;
    QWidget* fields_ = nullptr;
    VisitHistoryModel* history_ = nullptr;
    QTableView* historyTable_ = nullptr;
};

#endif // VR_GUI_PATIENT_RECORD_VIEW_HPP
