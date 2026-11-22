// NurseWindow.hpp - triage desk: register patients, watch both waiting
// lines, run the transfer desk.
//
//   Registration    TriageUnit (max-heap for emergencies, FIFO for OPD),
//                   PatientRegistry (hash + AVL), BillingEngine (bill opened)
//   Transfer desk   TransferDesk (deque)
#ifndef VR_GUI_NURSE_WINDOW_HPP
#define VR_GUI_NURSE_WINDOW_HPP

#include "RoleWindow.hpp"

class QTabWidget;
class RegistrationPanel;
class TransferDeskPanel;

class NurseWindow : public RoleWindow {
    Q_OBJECT
public:
    explicit NurseWindow(SessionHub& hub, QWidget* parent = nullptr);

    RegistrationPanel* registration() const { return registration_; }
    TransferDeskPanel* transfers() const { return transfers_; }
    QTabWidget* tabs() const { return tabs_; }

private:
    QTabWidget* tabs_ = nullptr;
    RegistrationPanel* registration_ = nullptr;
    TransferDeskPanel* transfers_ = nullptr;
};

#endif // VR_GUI_NURSE_WINDOW_HPP
