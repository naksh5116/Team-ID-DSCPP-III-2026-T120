// TreatPanel.hpp - "Treat next patient" (console option 3).
//
// Shows who is next (the top of the triage heap, else the head of the OPD
// line), takes the outpatient's notes when the next patient is an
// outpatient, and after treatment shows the doctor, the bed chosen by
// ResourceRouter (nearest free bed of the right type by Dijkstra distance)
// and the Navigator's walking directions. Used by Doctor and Super Admin.
#ifndef VR_GUI_TREAT_PANEL_HPP
#define VR_GUI_TREAT_PANEL_HPP

#include <QWidget>

class EsiBadge;
class QCheckBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTableView;
class RoleWindow;
class RoutePanel;
class RouteStepsView;
class SessionHub;
class EmergencyQueueModel;
class OpdQueueModel;

class TreatPanel : public QWidget {
    Q_OBJECT
public:
    TreatPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    // Treats the next patient. Returns false when nobody was treated.
    bool treatNext();

    QString resultText() const;   // the result lines, as shown
    RouteStepsView* route() const;                      // the directions list
    RoutePanel* routePanel() const { return route_; }   // map + directions
    QTableView* emergencyTable() const { return emergencyTable_; }
    // Where the route map is placed (piece 9); the steps list sits beside it.
    QWidget* routeArea() const { return routeArea_; }

Q_SIGNALS:
    void treated(const QString& patientId, bool bedAssigned);

private:
    void refreshNext();

    SessionHub& hub_;
    RoleWindow& host_;
    QLabel* nextId_ = nullptr;
    QLabel* nextName_ = nullptr;
    EsiBadge* nextBadge_ = nullptr;
    QLabel* nextKind_ = nullptr;
    QLabel* nextComplaint_ = nullptr;
    QLabel* nextNeeds_ = nullptr;
    QGroupBox* opdBox_ = nullptr;
    QLineEdit* diagnosis_ = nullptr;
    QLineEdit* prescriptions_ = nullptr;
    QCheckBox* dispense_ = nullptr;
    QLineEdit* medicine_ = nullptr;
    QSpinBox* quantity_ = nullptr;
    QDoubleSpinBox* unitPrice_ = nullptr;
    QPlainTextEdit* result_ = nullptr;
    QWidget* routeArea_ = nullptr;
    RoutePanel* route_ = nullptr;
    EmergencyQueueModel* emergencyModel_ = nullptr;
    OpdQueueModel* opdModel_ = nullptr;
    QTableView* emergencyTable_ = nullptr;
    QTableView* opdTable_ = nullptr;
};

#endif // VR_GUI_TREAT_PANEL_HPP
