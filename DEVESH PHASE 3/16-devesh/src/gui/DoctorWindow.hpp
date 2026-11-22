// DoctorWindow.hpp - treat patients, read and annotate records, find a free
// colleague, book theatres.
//
//   Treatment         TriageUnit (heap / FIFO), ResourceRouter (Dijkstra,
//                     type filter first), Navigator (directions),
//                     StaffRegistry (doctor booked), BillingEngine (charges)
//   Patient record    PatientRegistry (hash lookup), Patient visit list
//                     (doubly linked list, newest first)
//   Theatres          OTScheduler (circular queue of free slots)
//   Doctors on duty   StaffRegistry (sorted array, lower-bound search)
#ifndef VR_GUI_DOCTOR_WINDOW_HPP
#define VR_GUI_DOCTOR_WINDOW_HPP

#include "RoleWindow.hpp"

class DoctorAvailabilityPanel;
class PatientLookupPanel;
class QTabWidget;
class TheatrePanel;
class TreatPanel;

class DoctorWindow : public RoleWindow {
    Q_OBJECT
public:
    explicit DoctorWindow(SessionHub& hub, QWidget* parent = nullptr);

    QTabWidget* tabs() const { return tabs_; }
    TreatPanel* treatment() const { return treat_; }
    PatientLookupPanel* lookup() const { return lookup_; }
    TheatrePanel* theatres() const { return theatres_; }
    DoctorAvailabilityPanel* availability() const { return availability_; }

private:
    QTabWidget* tabs_ = nullptr;
    TreatPanel* treat_ = nullptr;
    PatientLookupPanel* lookup_ = nullptr;
    TheatrePanel* theatres_ = nullptr;
    DoctorAvailabilityPanel* availability_ = nullptr;
};

#endif // VR_GUI_DOCTOR_WINDOW_HPP
