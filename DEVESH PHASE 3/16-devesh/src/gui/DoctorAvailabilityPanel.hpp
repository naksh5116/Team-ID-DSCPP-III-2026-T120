// DoctorAvailabilityPanel.hpp - console option 6: next available doctor.
//
// StaffRegistry keeps doctors in an array sorted by (specialty, next-free
// time); a lower-bound binary search finds the soonest-free doctor of a
// specialty. Used by the Doctor and Super Admin windows.
#ifndef VR_GUI_DOCTOR_AVAILABILITY_PANEL_HPP
#define VR_GUI_DOCTOR_AVAILABILITY_PANEL_HPP

#include <QWidget>

class DoctorAvailabilityModel;
class QComboBox;
class QLabel;
class QTableView;
class RoleWindow;
class SessionHub;

class DoctorAvailabilityPanel : public QWidget {
    Q_OBJECT
public:
    DoctorAvailabilityPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    QString findNext();   // returns the result sentence
    QComboBox* specialty() const { return specialty_; }
    DoctorAvailabilityModel* model() const { return model_; }

private:
    void refreshSpecialties();

    SessionHub& hub_;
    RoleWindow& host_;
    QComboBox* specialty_ = nullptr;
    QLabel* result_ = nullptr;
    DoctorAvailabilityModel* model_ = nullptr;
    QTableView* table_ = nullptr;
};

#endif // VR_GUI_DOCTOR_AVAILABILITY_PANEL_HPP
