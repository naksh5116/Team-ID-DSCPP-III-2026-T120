// AdminWindow.hpp - Super Admin: every screen above, plus billing and
// discharge, admissions by date, the staff roster, the index figures, undo
// and save.
//
// Undo pops the session's undo journal (a vr_stack): the last registration
// or manual charge. Undo and save refresh every open window through the
// SessionHub.
#ifndef VR_GUI_ADMIN_WINDOW_HPP
#define VR_GUI_ADMIN_WINDOW_HPP

#include "RoleWindow.hpp"

class AdmissionsPanel;
class BillingPanel;
class IndexStatsPanel;
class PatientLookupPanel;
class QAction;
class QTabWidget;
class RegistrationPanel;
class StaffPanel;
class TheatrePanel;
class TransferDeskPanel;
class TreatPanel;

class AdminWindow : public RoleWindow {
    Q_OBJECT
public:
    explicit AdminWindow(SessionHub& hub, QWidget* parent = nullptr);

    // Asks for confirmation, then undoes the last entry. Returns false when
    // cancelled or there was nothing to undo.
    bool undoLast();

    QTabWidget* tabs() const { return tabs_; }
    RegistrationPanel* registration() const { return registration_; }
    TreatPanel* treatment() const { return treat_; }
    PatientLookupPanel* lookup() const { return lookup_; }
    BillingPanel* billing() const { return billing_; }
    AdmissionsPanel* admissions() const { return admissions_; }
    StaffPanel* staff() const { return staff_; }
    TheatrePanel* theatres() const { return theatres_; }
    TransferDeskPanel* transfers() const { return transfers_; }
    IndexStatsPanel* indexes() const { return indexes_; }

private:
    void updateUndoAction();

    QTabWidget* tabs_ = nullptr;
    QAction* undo_ = nullptr;
    RegistrationPanel* registration_ = nullptr;
    TreatPanel* treat_ = nullptr;
    PatientLookupPanel* lookup_ = nullptr;
    BillingPanel* billing_ = nullptr;
    AdmissionsPanel* admissions_ = nullptr;
    StaffPanel* staff_ = nullptr;
    TheatrePanel* theatres_ = nullptr;
    TransferDeskPanel* transfers_ = nullptr;
    IndexStatsPanel* indexes_ = nullptr;
};

#endif // VR_GUI_ADMIN_WINDOW_HPP
