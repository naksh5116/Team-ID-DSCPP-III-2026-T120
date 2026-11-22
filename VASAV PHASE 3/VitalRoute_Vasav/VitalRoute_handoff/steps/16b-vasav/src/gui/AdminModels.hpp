// AdminModels.hpp - tables only the Super Admin sees.
//
//   BillsListModel    every bill, merge-sorted by date opened or by amount
//                     (BillingEngine::listBills, vr_sort merge sort)
//   AdmissionsModel   patients registered between two dates
//                     (PatientRegistry::admittedBetween, AVL range walk)
//   StaffRosterModel  doctors, nurses and administrators (StaffRegistry)
#ifndef VR_GUI_ADMIN_MODELS_HPP
#define VR_GUI_ADMIN_MODELS_HPP

#include "SnapshotTableModel.hpp"

class BillsListModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit BillsListModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { BillId, PatientId, Opened, Total, State };
    void setSortByAmount(bool byAmount);
    bool sortByAmount() const { return byAmount_; }

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;   // key = patient ID

private:
    bool byAmount_ = false;
};

class AdmissionsModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit AdmissionsModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Registered, Id, Name, Kind, Status };
    void setRange(int fromYmd, int toYmd);

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;

private:
    int from_ = 0;
    int to_ = 0;
};

class StaffRosterModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit StaffRosterModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Id, Name, Role, Department, Shift, Detail };

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

#endif // VR_GUI_ADMIN_MODELS_HPP
