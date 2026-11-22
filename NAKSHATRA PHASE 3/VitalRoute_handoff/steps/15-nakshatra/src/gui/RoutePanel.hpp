// RoutePanel.hpp - the route map: one tab per floor, the directions beside.
//
// setRoute() gives every floor the Dijkstra path and the step-to-segment
// mapping. The floors the route crosses are marked in their tab titles.
// Selecting a step in the list switches to that step's floor and draws its
// segment wider (or rings the landmark for a turn).
#ifndef VR_GUI_ROUTE_PANEL_HPP
#define VR_GUI_ROUTE_PANEL_HPP

#include "Navigator.hpp"
#include "RouteSegments.hpp"

#include <QList>
#include <QWidget>

class FloorPlanWidget;
class QBoxLayout;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class RouteStepsView;
class SessionHub;

class RoutePanel : public QWidget {
    Q_OBJECT
public:
    // Horizontal: map left, steps right. Vertical: map above steps.
    RoutePanel(SessionHub& hub, Qt::Orientation orientation, QWidget* parent = nullptr);

    void setRoute(const Route& route);
    void clear(const QString& note = QString());
    void selectStep(int step);   // as if the step were clicked in the list

    // Wall display: larger directions and map labels.
    void setTextScale(double factor);
    void setOrientation(Qt::Orientation orientation);
    // Wall display: every floor the route crosses side by side (nobody
    // clicks tabs on a wall), instead of one tab per floor.
    void setWallMode(bool on);
    bool isWallMode() const { return wall_; }
    QList<FloorPlanWidget*> visibleWallFloors() const;

    RouteStepsView* steps() const { return steps_; }
    QTabWidget* floors() const { return floors_; }
    FloorPlanWidget* floorPlan(int floor) const;
    const RouteSegments& segments() const { return segments_; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void onStepSelected(int step);
    void retitleFloors();
    void updateWallFloors();
    void updateWallLayout();   // side by side or stacked, whichever gives the larger scale

    SessionHub& hub_;
    QSplitter* split_ = nullptr;
    QTabWidget* floors_ = nullptr;
    QStackedWidget* mapStack_ = nullptr;
    QList<FloorPlanWidget*> wallPlans_;
    QList<QWidget*> wallBoxes_;
    bool wall_ = false;
    QBoxLayout* wallRow_ = nullptr;
    QList<FloorPlanWidget*> plans_;
    RouteStepsView* steps_ = nullptr;
    Route route_;
    RouteSegments segments_;
};

#endif // VR_GUI_ROUTE_PANEL_HPP
