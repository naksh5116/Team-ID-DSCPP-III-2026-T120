// RegistrationForm.hpp - register an emergency patient or an outpatient.
//
// Same questions as console options 1 and 2. An emergency patient gets an
// ESI level (1-5, with the standard ESI names and decision points); an
// outpatient gets a department. Insurance is optional. Enter submits.
#ifndef VR_GUI_REGISTRATION_FORM_HPP
#define VR_GUI_REGISTRATION_FORM_HPP

#include <QWidget>

class InsuranceFields;
class QButtonGroup;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class RoleWindow;
class SessionHub;

class RegistrationForm : public QWidget {
    Q_OBJECT
public:
    enum class Kind { Emergency, Outpatient };

    RegistrationForm(SessionHub& hub, RoleWindow& host, Kind kind, QWidget* parent = nullptr);

    Kind kind() const { return kind_; }
    // Validates, registers through the hub, reports in the host's status bar.
    // Returns true when a patient was registered.
    bool submit();

Q_SIGNALS:
    void registered(const QString& patientId);

private:
    void refreshDepartments();
    void clear();

    SessionHub& hub_;
    RoleWindow& host_;
    Kind kind_;
    QLineEdit* name_ = nullptr;
    QSpinBox* age_ = nullptr;
    QLineEdit* phone_ = nullptr;
    QLineEdit* complaint_ = nullptr;
    QButtonGroup* esi_ = nullptr;         // emergency only, ids 1-5
    QComboBox* department_ = nullptr;     // outpatient only
    InsuranceFields* insurance_ = nullptr;
    QPushButton* submit_ = nullptr;
};

#endif // VR_GUI_REGISTRATION_FORM_HPP
