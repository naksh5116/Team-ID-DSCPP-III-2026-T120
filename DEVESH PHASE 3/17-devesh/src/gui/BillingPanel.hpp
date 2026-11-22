// BillingPanel.hpp - console options 9 and 10: view or edit a bill, record
// insurance, discharge and settle.
//
//   Manual charge   BillingEngine::manualCharge - only staff whose
//                   canAuthorizeCharges() is true (administrators); the
//                   charge is one of the four Charge subclasses, added
//                   with Bill::operator+=, and recorded for undo
//   Insurance       applied once, when the bill is settled
//   Discharge       further bed days charged, bed released, bill settled
//   All bills       BillingEngine::listBills, merge-sorted
#ifndef VR_GUI_BILLING_PANEL_HPP
#define VR_GUI_BILLING_PANEL_HPP

#include <QWidget>

class BillView;
class BillsListModel;
class InsuranceFields;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QStackedWidget;
class QTableView;
class RoleWindow;
class SessionHub;

class BillingPanel : public QWidget {
    Q_OBJECT
public:
    BillingPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    bool openBill(const QString& patientId);
    bool openBill();                 // the ID typed in the field
    bool addCharge();
    bool recordInsurance();
    bool discharge();                // asks for confirmation first

    QString patientId() const { return id_; }
    BillView* bill() const { return bill_; }
    BillsListModel* allBills() const { return bills_; }

private:
    void refreshAdmins();
    void updateEditable();

    SessionHub& hub_;
    RoleWindow& host_;
    QString id_;
    QLineEdit* idEdit_ = nullptr;
    QGroupBox* chargeBox_ = nullptr;
    QGroupBox* insuranceBox_ = nullptr;
    QGroupBox* dischargeBox_ = nullptr;
    QComboBox* authoriser_ = nullptr;
    QComboBox* type_ = nullptr;
    QLineEdit* description_ = nullptr;
    QStackedWidget* fields_ = nullptr;
    QDoubleSpinBox* bedRate_ = nullptr;
    QSpinBox* bedDays_ = nullptr;
    QDoubleSpinBox* medUnit_ = nullptr;
    QSpinBox* medQty_ = nullptr;
    QDoubleSpinBox* testPrice_ = nullptr;
    QCheckBox* testUrgent_ = nullptr;
    QDoubleSpinBox* procFee_ = nullptr;
    QDoubleSpinBox* procFacility_ = nullptr;
    QDoubleSpinBox* procConsumables_ = nullptr;
    InsuranceFields* insurance_ = nullptr;
    QDateEdit* dischargeDate_ = nullptr;
    BillView* bill_ = nullptr;
    BillsListModel* bills_ = nullptr;
    QTableView* billsTable_ = nullptr;
};

#endif // VR_GUI_BILLING_PANEL_HPP
