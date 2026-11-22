// BillView.hpp - one patient's bill, read-only.
//
// The charge lines are the bill's polymorphic Charge objects: each prices
// itself through calculateCost() and explains itself through detail(); the
// view never asks which kind of charge it is showing. Insurance is applied
// once, at settlement, so an open bill shows the deduction as an estimate.
// Used by the Patient window and (with editing beside it) Super Admin.
#ifndef VR_GUI_BILL_VIEW_HPP
#define VR_GUI_BILL_VIEW_HPP

#include "SnapshotTableModel.hpp"

#include <QWidget>

class QLabel;
class QTableView;

class BillLinesModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit BillLinesModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Date, Type, Description, Calculation, Amount };
    void setPatientId(const QString& id);

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;

private:
    QString id_;
};

class BillView : public QWidget {
    Q_OBJECT
public:
    explicit BillView(SessionHub& hub, QWidget* parent = nullptr);

    void setPatientId(const QString& id);
    QString patientId() const { return id_; }

    QString headerText() const;
    QString totalText() const;      // "Total due (estimate): Rs ..." / "Total paid: Rs ..."
    BillLinesModel* lines() const { return lines_; }

private:
    void refresh();

    SessionHub& hub_;
    QString id_;
    QLabel* header_ = nullptr;
    BillLinesModel* lines_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* subtotal_ = nullptr;
    QLabel* insurance_ = nullptr;
    QLabel* total_ = nullptr;
    QWidget* totals_ = nullptr;
};

#endif // VR_GUI_BILL_VIEW_HPP
