// PatientWindow.hpp - what a patient may see about themselves, read-only.
//
// The patient types their own ID (looked up in the hash table). The window
// shows their status, bed and visit history, their current bill, and the
// walking route from the Central Emergency Room to their bed (Dijkstra path
// turned into directions by the Navigator). It follows routeAssigned for
// its own patient, so the route appears the moment a doctor assigns a bed.
// Several Patient windows can be open for different patients.
#ifndef VR_GUI_PATIENT_WINDOW_HPP
#define VR_GUI_PATIENT_WINDOW_HPP

#include "RoleWindow.hpp"

class BillView;
class PatientRecordView;
class QLabel;
class QLineEdit;
class QSplitter;
class RoutePanel;
class RouteStepsView;

class PatientWindow : public RoleWindow {
    Q_OBJECT
public:
    explicit PatientWindow(SessionHub& hub, QWidget* parent = nullptr);

    // Shows the patient's record; false (and an error) for an unknown ID.
    bool showPatient(const QString& id);
    QString patientId() const { return id_; }

    PatientRecordView* record() const { return record_; }
    RouteStepsView* route() const;                      // the directions list
    RoutePanel* routePanel() const { return route_; }   // map + directions
    BillView* bill() const { return bill_; }
    // F11 wall display: only the floor map, large-type directions, distance
    // and walking time, under a header with the patient ID and destination
    // (no name: a wall display is public). Called when full screen changes.
    void setWallDisplay(bool on);
    bool isWallDisplay() const { return wall_; }
    QString wallHeaderText() const;

    // Where the route map is placed (piece 9), beside the directions.
    QWidget* routeArea() const { return routeArea_; }

protected:
    QString titleSuffix() const override { return id_; }
    void fullScreenChanged(bool fullScreen) override { setWallDisplay(fullScreen); }

private:
    void refreshRoute();
    void updateWallHeader();

    QString id_;
    QString shownRoute_;   // patient|bed|status the route was built for
    QLineEdit* idEdit_ = nullptr;
    PatientRecordView* record_ = nullptr;
    QWidget* routeArea_ = nullptr;
    RoutePanel* route_ = nullptr;
    BillView* bill_ = nullptr;
    QWidget* idRow_ = nullptr;
    QWidget* recordSection_ = nullptr;
    QWidget* routeSection_ = nullptr;
    QWidget* billSection_ = nullptr;
    QLabel* routeHeading_ = nullptr;
    QWidget* wallBar_ = nullptr;
    QLabel* wallHeader_ = nullptr;
    bool wall_ = false;
};

#endif // VR_GUI_PATIENT_WINDOW_HPP
